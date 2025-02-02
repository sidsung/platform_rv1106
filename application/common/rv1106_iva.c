#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <assert.h>
#include <semaphore.h>

#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/prctl.h>

#include <errno.h>
#include <pthread.h>
#include <sys/poll.h>
#include <stdatomic.h>

#include "rv1106_video_init.h"
#include "rv1106_common.h"
#include "rv1106_iva.h"
#include "graphics_Draw.h"


typedef struct {
    pthread_rwlock_t rwlock;
    uint32_t frameId;
    uint32_t objNum;                /* 目标个数 */
    RockIvaObjectInfo objInfo[128]; /* 各目标检测信息 */
} iva_detect_result_t;
typedef struct {
    iva_detect_result_t result[2];
} iva_detect_info_t;

static const char *iva_object_name[ROCKIVA_OBJECT_TYPE_MAX] = {
    "NONE",
    "PERSON",
    "VEHICLE",
    "NON_VEHICLE",
    "FACE",
    "HEAD",
    "PET",
    "MOTORCYCLE",
    "BICYCLE",
    "PLATE",
    "BABY",
    "PACKAGE",
};

static const uint32_t iva_object_color[ROCKIVA_OBJECT_TYPE_MAX] = {
    iva_black_color,   // "NONE"
    iva_red_color,     // "PERSON"
    iva_green_color,   // "VEHICLE"
    iva_blue_color,    // "NON_VEHICLE"
    iva_yellow_color,  // "FACE"
    iva_cyan_color,    // "HEAD"
    iva_magenta_color, // "PET"
    iva_orange_color,  // "MOTORCYCLE"
    iva_purple_color,  // "BICYCLE"
    iva_purple_color,  // "PLATE"
    iva_green_color,   // "BABY"
    iva_orange_color,  // "PACKAGE"
};

static iva_detect_info_t g_iva_detect_info = {0};

static iva_detect_result_t *get_iva_result_buffer(int write_flag)
{
    int min_id = 0;
    if (g_iva_detect_info.result[0].frameId <= g_iva_detect_info.result[1].frameId) {
        min_id = 0;
    } else {
        min_id = 1;
    }

    if (write_flag == 0) {
        min_id = 1 - min_id;
        pthread_rwlock_rdlock(&g_iva_detect_info.result[min_id].rwlock);
    } else {
        pthread_rwlock_wrlock(&g_iva_detect_info.result[min_id].rwlock);
    }

    return &g_iva_detect_info.result[min_id];
}

// static RK_U64 TEST_COMM_GetNowUs() {
//     struct timespec time = {0, 0};
//     clock_gettime(CLOCK_MONOTONIC, &time);
//     return (RK_U64)time.tv_sec * 1000000 + (RK_U64)time.tv_nsec / 1000; /* microseconds */
// }

static void FrameReleaseCallback(const RockIvaReleaseFrames* releaseFrames, void* userdata)
{
    // video_iva_param_t* iva_ctx = (video_iva_param_t*)userdata;
    // printf("FrameReleaseCallback count=%d\n", releaseFrames->count);
}

static void DetResultCallback(const RockIvaDetectResult* result, const RockIvaExecuteStatus status, void* userdata)
{
    // video_iva_param_t* iva_ctx = (video_iva_param_t*)userdata;
    iva_detect_result_t *det_result = get_iva_result_buffer(1);
    det_result->objNum = result->objNum;
    det_result->frameId = result->frameId;
    memcpy(det_result->objInfo, result->objInfo, sizeof(RockIvaObjectInfo) * det_result->objNum);
    pthread_rwlock_unlock(&det_result->rwlock);

    // static uint64_t last_timestamp = 0;
    // uint64_t new_timestamp = TEST_COMM_GetNowUs();
    // printf("IVA result ---> seq:%d delay:%dms fps:%.1f\n", ctx->objInfo[0].frameId, (uint32_t)(new_timestamp - last_timestamp) / 1000, (1000.0 / ((new_timestamp - last_timestamp) / 1000)));
    // last_timestamp = new_timestamp;
}

int rv1106_iva_get_result(smart_detect_result_obj_item_t *item)
{
    int ret = 0;

    iva_detect_result_t *result = get_iva_result_buffer(0);
    item->object_number = 0;
    item->frameId = result->frameId;

    for (int i = 0; i < (int)result->objNum; i++) {
        if ((uint16_t)(result->objInfo[i].rect.topLeft.x) >= 10000 || (uint16_t)result->objInfo[i].rect.topLeft.y >= 10000
            || (uint16_t)result->objInfo[i].rect.bottomRight.x >= 10000 || (uint16_t)result->objInfo[i].rect.bottomRight.y >= 10000) {
            continue;
        }

        if (item->object_number < SMART_DETECT_ITEM_NUM) {
            item->obj_item[item->object_number].type_index = result->objInfo[i].type;
            item->obj_item[item->object_number].score = result->objInfo[i].score;

            snprintf(item->obj_item[item->object_number].object_name, sizeof(item->obj_item[item->object_number].object_name), "%s", iva_object_name[result->objInfo[i].type]);
            item->obj_item[item->object_number].name_color = iva_object_color[result->objInfo[i].type];

            item->obj_item[item->object_number].x1 = result->objInfo[i].rect.topLeft.x;
            item->obj_item[item->object_number].y1 = result->objInfo[i].rect.topLeft.y;
            item->obj_item[item->object_number].x2 = result->objInfo[i].rect.bottomRight.x;
            item->obj_item[item->object_number].y2 = result->objInfo[i].rect.bottomRight.y;
            item->obj_item[item->object_number].rect_color = iva_object_color[result->objInfo[i].type];
        }
        item->object_number++;
    }
    pthread_rwlock_unlock(&result->rwlock);

    return ret;
}

int rv1106_iva_push_frame(video_iva_param_t *iva, frameInfo_vi_t *Fvi_info)
{
    uint64_t frame_size = 0;
    RockIvaRetCode s32Ret = RK_FAILURE;
    RockIvaImage image = {0};

    do {

        image.channelId = 0;
        image.frameId = Fvi_info->frame_seq;
        image.info.width = Fvi_info->width;
        image.info.height = Fvi_info->height;
        image.info.transformMode = ROCKIVA_IMAGE_TRANSFORM_NONE;

        switch (Fvi_info->PixelFormat) {
            case RK_FMT_RGB888: image.info.format = ROCKIVA_IMAGE_FORMAT_RGB888; frame_size = Fvi_info->width * Fvi_info->height * 3; break;
            case RK_FMT_BGR888: image.info.format = ROCKIVA_IMAGE_FORMAT_BGR888; frame_size = Fvi_info->width * Fvi_info->height * 3; break;
            case RK_FMT_RGBA8888: image.info.format = ROCKIVA_IMAGE_FORMAT_RGBA8888; frame_size = Fvi_info->width * Fvi_info->height * 4; break;
            case RK_FMT_BGRA8888: image.info.format = ROCKIVA_IMAGE_FORMAT_BGRA8888; frame_size = Fvi_info->width * Fvi_info->height * 4;  break;
            case RK_FMT_YUV420SP: image.info.format = ROCKIVA_IMAGE_FORMAT_YUV420SP_NV12; frame_size = Fvi_info->width * Fvi_info->height * 1.5;  break;
            default: {
                printf("[%s %d] unsupport PixelFormat:%d\n", __func__, __LINE__, Fvi_info->PixelFormat);
                s32Ret = ROCKIVA_RET_UNSUPPORTED;
                break;
            }
        }
        if (s32Ret == ROCKIVA_RET_UNSUPPORTED) {
            break;
        }

        if (image.dataAddr == NULL) {
            image.dataAddr = (unsigned char *)malloc(frame_size);
            if (image.dataAddr == NULL) {
                printf("[%s %d] malloc error\n", __func__, __LINE__);
                break;
            }
        }

        image.size = frame_size;
        memmove(image.dataAddr, Fvi_info->frame_data, frame_size);

        s32Ret = ROCKIVA_PushFrame(iva->handle, &image, NULL);
        if (s32Ret != ROCKIVA_RET_SUCCESS) {
            printf("[%s %d] ROCKIVA_PushFrame error: ret:%d\n", __func__, __LINE__, s32Ret);
            break;
        }
    } while (0);

    if (s32Ret != ROCKIVA_RET_SUCCESS) {
        if (image.dataAddr != NULL) {
            free(image.dataAddr);
            image.dataAddr = NULL;
        }
    }

    return s32Ret;
}

int rv1106_iva_push_frame_fd(video_iva_param_t *iva, frameInfo_vi_t *Fvi_info)
{
    RockIvaRetCode s32Ret = RK_FAILURE;
    RockIvaImage image = {0};
    // static uint64_t count = 0;
    do {

        image.channelId = 0;
        image.dataFd = Fvi_info->dataFd;
        image.frameId = Fvi_info->frame_seq;
        // image.frameId = count++;
        image.info.width = Fvi_info->width;
        image.info.height = Fvi_info->height;
        image.info.transformMode = ROCKIVA_IMAGE_TRANSFORM_NONE;

        switch (Fvi_info->PixelFormat) {
            case RK_FMT_RGB888: image.info.format = ROCKIVA_IMAGE_FORMAT_RGB888; break;
            case RK_FMT_BGR888: image.info.format = ROCKIVA_IMAGE_FORMAT_BGR888; break;
            case RK_FMT_RGBA8888: image.info.format = ROCKIVA_IMAGE_FORMAT_RGBA8888; break;
            case RK_FMT_BGRA8888: image.info.format = ROCKIVA_IMAGE_FORMAT_BGRA8888; break;
            case RK_FMT_YUV420SP: image.info.format = ROCKIVA_IMAGE_FORMAT_YUV420SP_NV12; break;
            default: {
                printf("[%s %d] unsupport PixelFormat:%d\n", __func__, __LINE__, Fvi_info->PixelFormat);
                s32Ret = ROCKIVA_RET_UNSUPPORTED;
                break;
            }
        }
        if (s32Ret == ROCKIVA_RET_UNSUPPORTED) {
            break;
        }

        s32Ret = ROCKIVA_PushFrame(iva->handle, &image, NULL);
        if (s32Ret != ROCKIVA_RET_SUCCESS) {
            printf("[%s %d] ROCKIVA_PushFrame error: ret:%d\n", __func__, __LINE__, s32Ret);
            break;
        }
    } while (0);

    return s32Ret;
}

int rv1106_iva_init(video_iva_param_t *iva)
{
    RockIvaRetCode s32Ret = RK_FAILURE;
    RockIvaDetTaskParams detParams;
    RockIvaInitParam ivaParams;

    char version[32] = {0};
    if (iva->enable == 0) {
        printf("[%s %d] error: iva not enable\n", __func__, __LINE__);
        return s32Ret;
    }

    s32Ret = ROCKIVA_GetVersion(sizeof(version), version);
    if (s32Ret != ROCKIVA_RET_SUCCESS) {
        printf("[%s %d] ROCKIVA_SetFrameReleaseCallback error: ret:%d\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    printf("ROCKIVA Version:%*.s\n", sizeof(version), version);

    memset(&ivaParams, 0, sizeof(RockIvaInitParam));
    memset(&detParams, 0, sizeof(RockIvaDetTaskParams));

    // 配置模型路径
    snprintf(ivaParams.modelPath, ROCKIVA_PATH_LENGTH, iva->models_path);
    ivaParams.coreMask = 0x04;
    ivaParams.logLevel = ROCKIVA_LOG_ERROR;

    ivaParams.detModel = ROCKIVA_DET_MODEL_PFCP;

    ivaParams.imageInfo.width = iva->width;
    ivaParams.imageInfo.height = iva->height;
    ivaParams.imageInfo.format = iva->IvaPixelFormat;
    ivaParams.imageInfo.transformMode = ROCKIVA_IMAGE_TRANSFORM_NONE;

    do {
        s32Ret = ROCKIVA_Init(&iva->handle, ROCKIVA_MODE_VIDEO, &ivaParams, iva);
        if (s32Ret != ROCKIVA_RET_SUCCESS) {
            printf("[%s %d] ROCKIVA_Init error: ret:%d\n", __func__, __LINE__, s32Ret);
            break;
        }

        s32Ret = ROCKIVA_SetFrameReleaseCallback(iva->handle, FrameReleaseCallback);
        if (s32Ret != ROCKIVA_RET_SUCCESS) {
            printf("[%s %d] ROCKIVA_SetFrameReleaseCallback error: ret:%d\n", __func__, __LINE__, s32Ret);
            break;
        }

        // // 设置上报目标类型
        // detParams.detObjectType |= ROCKIVA_OBJECT_TYPE_BITMASK(ROCKIVA_OBJECT_TYPE_PERSON);
        // detParams.detObjectType |= ROCKIVA_OBJECT_TYPE_BITMASK(ROCKIVA_OBJECT_TYPE_FACE);
        // detParams.detObjectType |= ROCKIVA_OBJECT_TYPE_BITMASK(ROCKIVA_OBJECT_TYPE_PET);

        // // 设置检测分数阈值，只设置第 0 个可以对所有类别生效
        // detParams.scores[0] = 30;
        // detParams.min_det_count = 2;

        s32Ret = ROCKIVA_DETECT_Init(iva->handle, &detParams, DetResultCallback);
        if (s32Ret != ROCKIVA_RET_SUCCESS) {
            printf("[%s %d] ROCKIVA_DETECT_Init error: ret:%d\n", __func__, __LINE__, s32Ret);
            break;
        }

        for (int i = 0; i < (int)(sizeof(g_iva_detect_info.result) / sizeof(g_iva_detect_info.result[0])); i++) {
            pthread_rwlock_init(&g_iva_detect_info.result[i].rwlock, NULL);
        }

        s32Ret = 0;
    } while(0);
    return s32Ret;
}

int rv1106_iva_deinit(video_iva_param_t *iva)
{
    RockIvaRetCode s32Ret = RK_FAILURE;

    if (iva->enable == 0) {
        printf("[%s %d] error: iva not enable\n", __func__, __LINE__);
        return s32Ret;
    }

    for (int i = 0; i < (int)(sizeof(g_iva_detect_info.result) / sizeof(g_iva_detect_info.result[0])); i++) {
        pthread_rwlock_destroy(&g_iva_detect_info.result[i].rwlock);
    }

    s32Ret = ROCKIVA_WaitFinish(iva->handle, -1, 3000);
    if (s32Ret != ROCKIVA_RET_SUCCESS) {
        printf("[%s %d] ROCKIVA_WaitFinish error: ret:%d\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    s32Ret = ROCKIVA_DETECT_Release(iva->handle);
    if (s32Ret != ROCKIVA_RET_SUCCESS) {
        printf("[%s %d] ROCKIVA_DETECT_Release error: ret:%d\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    s32Ret = ROCKIVA_Release(iva->handle);
    if (s32Ret != ROCKIVA_RET_SUCCESS) {
        printf("[%s %d] ROCKIVA_Release error: ret:%d\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
