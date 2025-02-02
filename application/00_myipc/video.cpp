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

#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/prctl.h>

#include <errno.h>
#include <pthread.h>
#include <sys/poll.h>
#include <semaphore.h>

#include "main.h"

#include "im2d.h"
#include "RgaUtils.h"

#include "graphics_Draw.h"
#include "rv1106_video_init.h"

#include "dma_alloc.h"

#if CONFIG_ENABLE_ROCKCHIP_IVA
#include "rv1106_iva.h"
#endif

#if CONFIG_ENABLE_SCREEN_PANEL
#include "screen_panel_driver.h"
#endif

#include "init_param.h"
#include "video.h"

#include "osd.h"
#include "utils.h"

pthread_t g_video_main_thread_id;
pthread_t g_video_sub_thread_id;
pthread_t g_video_third_thread_id;

static int g_video_capture_thread_run = 1;

#if CONFIG_ENABLE_ROCKCHIP_IVA
static void *iva_push_frame_thread(void *pArgs)
{
    int s32Ret = -1;
    video_iva_param_t *iva = get_iva_param();
    video_vi_chn_param_t *vi_chn = get_vi_chn_param();

    VIDEO_FRAME_INFO_S stViFrame;

    RockIvaImage image = {0};

    // // 设置线程为可取消的
    // pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    // // 设置取消类型为延迟取消
    // pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    // // 注册清理函数
    // pthread_cleanup_push(thread_cleanup, NULL);

    printf("[%s %d] Start iva push stream thread......\n", __FILE__, __LINE__);

    vi_chn = &vi_chn[1];
    while (iva->push_thread_run) {
        do {
            s32Ret = RK_MPI_VI_GetChnFrame(vi_chn->ViPipe, vi_chn->viChnId, &stViFrame, 1000);
            if (s32Ret != RK_SUCCESS) {
                printf("[%s %d] error: RK_MPI_VI_GetChnFrame fail: ret:0x%X\n", __func__, __LINE__, s32Ret);
                break;
            }

            int mpi_src_fd = RK_MPI_MB_Handle2Fd(stViFrame.stVFrame.pMbBlk);

            image.channelId = 0;
            image.dataFd = mpi_src_fd;
            image.frameId = stViFrame.stVFrame.u32TimeRef;

            image.info.width = stViFrame.stVFrame.u32VirWidth;
            image.info.height = stViFrame.stVFrame.u32VirHeight;
            image.info.transformMode = ROCKIVA_IMAGE_TRANSFORM_NONE;

            image.info.format = ROCKIVA_IMAGE_FORMAT_YUV420SP_NV12;

            s32Ret = ROCKIVA_PushFrame(iva->handle, &image, NULL);
            if (s32Ret != ROCKIVA_RET_SUCCESS) {
                printf("[%s %d] ROCKIVA_PushFrame error: ret:%d\n", __func__, __LINE__, s32Ret);
                break;
            }

            // static uint64_t last_timestamp = 0;
            // printf("iva ---> seq:%d w:%d h:%d fmt:%d size:%lld delay:%dms fps:%.1f\n", stViFrame.stVFrame.u32TimeRef, stViFrame.stVFrame.u32Width, stViFrame.stVFrame.u32Height, stViFrame.stVFrame.enPixelFormat, stViFrame.stVFrame.u64PrivateData, (uint32_t)(stViFrame.stVFrame.u64PTS - last_timestamp) / 1000, (1000.0 / ((stViFrame.stVFrame.u64PTS - last_timestamp) / 1000)));
            // last_timestamp = stViFrame.stVFrame.u64PTS;

            s32Ret = RK_MPI_VI_ReleaseChnFrame(vi_chn->ViPipe, vi_chn->viChnId, &stViFrame);
            if (s32Ret != RK_SUCCESS) {
                printf("error: RK_MPI_VI_ReleaseChnFrame fail chn:%d 0x%X\n", vi_chn->viChnId, s32Ret);
            }

        } while (0);
    }

    // 移除清理函数
    // pthread_cleanup_pop(1);

    return RK_NULL;
}
#endif

static void *video_main_thread(void *pArgs)
{
    int s32Ret = -1;
    video_vi_chn_param_t *vi_chn = get_vi_chn_param();
    video_vpss_param_t *vpss = get_vpss_param();

    VIDEO_FRAME_INFO_S stViFrame;
    uint8_t* frame_data = NULL;
    int mpi_src_fd;

    printf("[%s %d] Start video main stream thread......\n", __FILE__, __LINE__);

    vi_chn = &vi_chn[0];

    while (g_video_capture_thread_run) {
        mpi_src_fd = 0;
        do {
            s32Ret = RK_MPI_VI_GetChnFrame(vi_chn->ViPipe, vi_chn->viChnId, &stViFrame, 1000);
            if (s32Ret != RK_SUCCESS) {
                printf("[%s %d] error: RK_MPI_VI_GetChnFrame fail: ret:0x%X\n", __func__, __LINE__, s32Ret);
                break;
            }

            mpi_src_fd = RK_MPI_MB_Handle2Fd(stViFrame.stVFrame.pMbBlk);
            frame_data = (uint8_t*)RK_MPI_MB_Handle2VirAddr(stViFrame.stVFrame.pMbBlk);

            rga_buffer_handle_t src_handle = importbuffer_fd(mpi_src_fd, stViFrame.stVFrame.u64PrivateData);
            rga_buffer_t src_rga_buffer = wrapbuffer_handle(src_handle, stViFrame.stVFrame.u32Width, stViFrame.stVFrame.u32Height, RK_FORMAT_YCbCr_420_SP);

            update_osd(&stViFrame, src_rga_buffer, frame_data);

            s32Ret = RK_MPI_VPSS_SendFrame(vpss->VpssGrpID, 0, &stViFrame, 1000);
            if (s32Ret != RK_SUCCESS) {
                printf("[%s %d] error: RK_MPI_VPSS_SendFrame fail: ret:0x%X\n", __func__, __LINE__, s32Ret);
                // break;
            }

            // if (stViFrame.stVFrame.u32TimeRef > 20 && stViFrame.stVFrame.u32TimeRef < 25) {
            //     utils_write_file("./main.bin", frame_data, stViFrame.stVFrame.u64PrivateData);
            // }

            // static uint64_t last_timestamp = 0;
            // printf("main ---> seq:%d w:%d h:%d fmt:%d size:%lld delay:%dms fps:%.1f\n", stViFrame.stVFrame.u32TimeRef, stViFrame.stVFrame.u32Width, stViFrame.stVFrame.u32Height, stViFrame.stVFrame.enPixelFormat, stViFrame.stVFrame.u64PrivateData, (uint32_t)(stViFrame.stVFrame.u64PTS - last_timestamp) / 1000, (1000.0 / ((stViFrame.stVFrame.u64PTS - last_timestamp) / 1000)));
            // last_timestamp = stViFrame.stVFrame.u64PTS;

            if (src_handle)
                releasebuffer_handle(src_handle);

            s32Ret = RK_MPI_VI_ReleaseChnFrame(vi_chn->ViPipe, vi_chn->viChnId, &stViFrame);
            if (s32Ret != RK_SUCCESS) {
                printf("error: RK_MPI_VI_ReleaseChnFrame fail chn:%d 0x%X\n", vi_chn->viChnId, s32Ret);
            }
        } while (0);
    }
    return RK_NULL;
}

#if CONFIG_ENABLE_SCREEN_PANEL
static void *video_third_thread(void *pArgs)
{
    int s32Ret = -1;
    VIDEO_FRAME_INFO_S stViFrame;
    PIC_BUF_ATTR_S stPicBufAttr;
    MB_PIC_CAL_S stMbPicCalResult;
    rga_buffer_handle_t src_rga_handle = 0;

    video_vpss_param_t *vpss = get_vpss_param();
    video_vpss_chn_param_t *vpss_chn = &vpss->chn[0];

    printf("[%s %d] Start video third stream thread......\n", __FILE__, __LINE__);

    while (g_video_capture_thread_run) {
        src_rga_handle = 0;
        do {
            s32Ret = RK_MPI_VPSS_GetChnFrame(vpss->VpssGrpID, vpss_chn->VpssChnID, &stViFrame, 1000);
            if (s32Ret != RK_SUCCESS) {
                printf("[%s %d] error: RK_MPI_VPSS_GetChnFrame fail: ret:0x%X\n", __func__, __LINE__, s32Ret);
                break;
            }

            stPicBufAttr.u32Width = stViFrame.stVFrame.u32VirWidth;
            stPicBufAttr.u32Height = stViFrame.stVFrame.u32VirHeight;
            stPicBufAttr.enPixelFormat = stViFrame.stVFrame.enPixelFormat;
            stPicBufAttr.enCompMode = stViFrame.stVFrame.enCompressMode;
            s32Ret = RK_MPI_CAL_VGS_GetPicBufferSize(&stPicBufAttr, &stMbPicCalResult);
            if (s32Ret != RK_SUCCESS) {
                printf("[%s %d] error: RK_MPI_CAL_VGS_GetPicBufferSize fail: ret:0x%X\n", __func__, __LINE__, s32Ret);
                break;
            }
            stViFrame.stVFrame.u64PrivateData = stMbPicCalResult.u32MBSize;

            // static uint64_t last_timestamp = 0;
            // printf("third ---> seq:%d w:%d h:%d fmt:%d size:%lld delay:%dms fps:%.1f\n", stViFrame.stVFrame.u32TimeRef, stViFrame.stVFrame.u32Width, stViFrame.stVFrame.u32Height, stViFrame.stVFrame.enPixelFormat, stViFrame.stVFrame.u64PrivateData, (uint32_t)(stViFrame.stVFrame.u64PTS - last_timestamp) / 1000, (1000.0 / ((stViFrame.stVFrame.u64PTS - last_timestamp) / 1000)));
            // last_timestamp = stViFrame.stVFrame.u64PTS;

            int src_fd = RK_MPI_MB_Handle2Fd(stViFrame.stVFrame.pMbBlk);
            src_rga_handle = importbuffer_fd(src_fd, stViFrame.stVFrame.u64PrivateData);
            rga_buffer_t src_rga_img = wrapbuffer_handle(src_rga_handle, stViFrame.stVFrame.u32Width, stViFrame.stVFrame.u32Height, RK_FORMAT_YCbCr_420_SP);

            screen_panel_param_t *screen_panel = get_screen_panel_param();
            rga_buffer_handle_t screen_handle = importbuffer_virtualaddr(screen_panel->fb_dev.screen_base, screen_panel->fb_dev.screen_size);
            rga_buffer_t screen_img = wrapbuffer_handle(screen_handle, screen_panel->fb_dev.width, screen_panel->fb_dev.height, RK_FORMAT_BGRA_8888);

            s32Ret = imcvtcolor(src_rga_img, screen_img, RK_FORMAT_YCbCr_420_SP, RK_FORMAT_BGRA_8888);
            if (s32Ret != IM_STATUS_SUCCESS) {
                printf("%s running failed, %s\n", "imcvtcolor", imStrError((IM_STATUS)s32Ret));
            }

        } while (0);

        if (src_rga_handle > 0)
            releasebuffer_handle(src_rga_handle);

        s32Ret = RK_MPI_VPSS_ReleaseChnFrame(vpss->VpssGrpID, vpss_chn->VpssChnID, &stViFrame);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: RK_MPI_VPSS_ReleaseChnFrame fail: ret:0x%X\n", __func__, __LINE__, s32Ret);
        }
    }
    return RK_NULL;
}
#endif

int video_GetFrame(get_frame_type_t type, frameInfo_vi_t *fvi_info, void *arg)
{
    RK_S32 s32Ret = RK_FAILURE;

    if (type == GET_VENC_FRAME) {
        video_venc_param_t *venc = get_venc_param();
        s32Ret = rv1106_venc_GetStream(&venc[0], fvi_info);
        if (s32Ret != RK_SUCCESS) {
            printf("error: GET_VENC_FRAME fail: ret:0x%x\n", s32Ret);
            return s32Ret;
        }
    }
    else if (type == GET_VENC2_FRAME) {
        video_venc_param_t *venc = get_venc_param();
        s32Ret = rv1106_venc_GetStream(&venc[1], fvi_info);
        if (s32Ret != RK_SUCCESS) {
            printf("error: GET_VENC2_FRAME fail: ret:0x%x\n", s32Ret);
            return s32Ret;
        }
    }

    return s32Ret;
}

int video_init(void)
{
    RK_S32 s32Ret = RK_FAILURE;

    init_video_param_list();

    s32Ret = rv1106_video_init(get_video_param_list());
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: rv1106_video_init fail: ret:0x%x\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

#if CONFIG_ENABLE_ROCKCHIP_IVA
    video_iva_param_t *iva = get_iva_param();
    if (iva->enable) {
        iva->push_thread_run = true;
        s32Ret = rv1106_iva_init(iva);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: rv1106_iva_init s32Ret:0x%X\n", __func__, __LINE__, s32Ret);
            return s32Ret;
        }
        pthread_create(&iva->push_thread_id, 0, iva_push_frame_thread, NULL);
    }
#endif

    pthread_create(&g_video_main_thread_id, 0, video_main_thread, NULL);
    // pthread_create(&g_video_sub_thread_id, 0, video_sub_thread, NULL);

#if CONFIG_ENABLE_SCREEN_PANEL
    pthread_create(&g_video_third_thread_id, 0, video_third_thread, NULL);
#endif

    return s32Ret;
}

int video_deinit(void)
{
    RK_S32 s32Ret = RK_FAILURE;

    g_video_capture_thread_run = 0;

    if (g_video_main_thread_id) {
        printf("[%s %d] wait main thread join\n", __FILE__, __LINE__);
        pthread_join(g_video_main_thread_id, NULL);
    }

    if (g_video_sub_thread_id) {
        printf("[%s %d] wait main thread join\n", __FILE__, __LINE__);
        pthread_join(g_video_sub_thread_id, NULL);
    }

    if (g_video_third_thread_id) {
        printf("[%s %d] wait third thread join\n", __FILE__, __LINE__);
        pthread_join(g_video_third_thread_id, NULL);
    }

#if CONFIG_ENABLE_ROCKCHIP_IVA
    video_iva_param_t *iva = get_iva_param();
    if (iva->enable) {
        iva->push_thread_run = false;
        if (iva->push_thread_id) {
            printf("[%s %d] wait iva thread joid\n", __FILE__, __LINE__);
            // pthread_cancel(iva.push_thread_id);
            pthread_join(iva->push_thread_id, NULL);
        }

        printf(">>> rv1106_iva_deinit \n");
        s32Ret = rv1106_iva_deinit(iva);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: rv1106_iva_deinit s32Ret:0x%X\n", __func__, __LINE__, s32Ret);
            return s32Ret;
        }
        printf("rv1106_iva_deinit OK\n");
    }
#endif

    rv1106_video_init_param_t *video_param = get_video_param_list();
    s32Ret = rv1106_video_deinit(video_param);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: rv1106_video_deinit fail: ret:0x%x\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    return s32Ret;
}
