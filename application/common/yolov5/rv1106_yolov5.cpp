#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "rv1106_yolov5.h"
#include "postprocess.h"

static pthread_rwlock_t g_rwlock;
static object_detect_result_list_t g_od_results = {0};

#define iva_red_color        0xFFFF0000    // 红色
#define iva_green_color      0xFF00FF00    // 绿色
#define iva_blue_color       0xFF0000FF    // 蓝色
#define iva_yellow_color     0xFFFFFF00    // 黄色
#define iva_cyan_color       0xFF00FFFF    // 青色
#define iva_magenta_color    0xFFFF00FF    // 品红
#define iva_orange_color     0xFFFFA500    // 橙色
#define iva_purple_color     0xFF800080    // 紫色
#define iva_white_color      0xFFFFFFFF    // 白色
#define iva_black_color      0xFF000000    // 黑色

int rv1106_yolov5_get_result(smart_detect_result_obj_item_t *item)
{
    int ret = 0;

    item->object_number = 0;

    pthread_rwlock_rdlock(&g_rwlock);
    item->frameId = g_od_results.id;

    for (int i = 0; i < g_od_results.count; i++) {
        if (g_od_results.results[i].box.left >= 640
            || g_od_results.results[i].box.top >= 640
            || g_od_results.results[i].box.right >= 640
            || g_od_results.results[i].box.bottom >= 640)
        {
            continue;
        }

        if (item->object_number < SMART_DETECT_ITEM_NUM) {
            item->obj_item[item->object_number].type_index = g_od_results.results[i].cls_id;
            item->obj_item[item->object_number].score = g_od_results.results[i].prop * 100;

            snprintf(item->obj_item[item->object_number].object_name, sizeof(item->obj_item[item->object_number].object_name), "%s", coco_cls_to_name(g_od_results.results[i].cls_id));

            item->obj_item[item->object_number].name_color = iva_green_color;

            item->obj_item[item->object_number].x1 = g_od_results.results[i].box.left * 10000 / 640;
            item->obj_item[item->object_number].y1 = g_od_results.results[i].box.top * 10000 / 640;
            item->obj_item[item->object_number].x2 = g_od_results.results[i].box.right * 10000 / 640;
            item->obj_item[item->object_number].y2 = g_od_results.results[i].box.bottom * 10000 / 640;
            item->obj_item[item->object_number].rect_color = iva_red_color;
        }
        item->object_number++;
    }

    pthread_rwlock_unlock(&g_rwlock);

    return ret;
}

int rv1106_yolov5_inference(yolov5_init_param_t *yolo, rga_buffer_t src_rga_buffer, uint32_t frameId)
{
    RK_S32 s32Ret = RK_FAILURE;

    object_detect_result_list_t od_results = {0};

    int rknn_buf_size = 640 * 640 * 3;

    rga_buffer_handle_t rga_rknn_handle = importbuffer_fd(yolo->rknn_ctx.input_mems[0]->fd, rknn_buf_size);
    rga_buffer_t rga_rknn_img = wrapbuffer_handle(rga_rknn_handle, 640, 640, RK_FORMAT_BGR_888);

    do {
        s32Ret = imcvtcolor(src_rga_buffer, rga_rknn_img, RK_FORMAT_YCbCr_420_SP, RK_FORMAT_BGR_888);
        if (s32Ret != IM_STATUS_SUCCESS) {
            printf("%s running failed, %s\n", "imcvtcolor", imStrError((IM_STATUS)s32Ret));
            break;
        }

        s32Ret = inference_model(&yolo->rknn_ctx, &od_results);
        if (s32Ret < 0) {
            printf("inference_model fail! ret=%d\n", s32Ret);
            break;
        }
        od_results.id = frameId;

        // printf("id:%d count:%d\n", od_results.id, od_results.count);
        // if (od_results.count) {
        //     printf("cls_id:%d prop:%.2f %d %d %d %d\n", od_results.results[0].cls_id, od_results.results[0].prop, od_results.results[0].box.left, od_results.results[0].box.top, od_results.results[0].box.right, od_results.results[0].box.bottom);
        // }

        pthread_rwlock_wrlock(&g_rwlock);
        memmove(&g_od_results, &od_results, sizeof(object_detect_result_list_t));
        pthread_rwlock_unlock(&g_rwlock);

    } while (0);

    if (rga_rknn_handle)
        releasebuffer_handle(rga_rknn_handle);

    return s32Ret;
}

int rv1106_yolov5_init(yolov5_init_param_t *yolo)
{
    RK_S32 s32Ret = RK_FAILURE;

    yolo->rknn_ctx.model_path = yolo->model_path;

    pthread_rwlock_init(&g_rwlock, NULL);

    s32Ret = rknn_app_init(&yolo->rknn_ctx);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: rknn_app_init fail: ret:0x%x\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    s32Ret = init_post_process(yolo->labels_file);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: init_post_process fail: ret:0x%x\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

int rv1106_yolov5_deinit(yolov5_init_param_t *yolo)
{
    RK_S32 s32Ret = RK_FAILURE;

    deinit_post_process();
    rknn_app_deinit(&yolo->rknn_ctx);

    pthread_rwlock_destroy(&g_rwlock);

    s32Ret = 0;
    return s32Ret;
}
