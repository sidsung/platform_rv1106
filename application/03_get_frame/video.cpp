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

#include "im2d.h"
#include "RgaUtils.h"

#include "rv1106_video_init.h"

#include "init_param.h"
#include "video.h"

#include "utils.h"

pthread_t g_video_main_thread_id;
pthread_t g_video_sub_thread_id;
pthread_t g_video_third_thread_id;

static int g_video_capture_thread_run = 1;

static void *video_main_thread(void *pArgs)
{
    int s32Ret = -1;
    video_vi_chn_param_t *vi_chn = get_vi_chn_param();

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

            static uint64_t last_timestamp = 0;
            printf("main ---> seq:%d w:%d h:%d fmt:%d size:%lld delay:%dms fps:%.1f\n", stViFrame.stVFrame.u32TimeRef, stViFrame.stVFrame.u32Width, stViFrame.stVFrame.u32Height, stViFrame.stVFrame.enPixelFormat, stViFrame.stVFrame.u64PrivateData, (uint32_t)(stViFrame.stVFrame.u64PTS - last_timestamp) / 1000, (1000.0 / ((stViFrame.stVFrame.u64PTS - last_timestamp) / 1000)));
            last_timestamp = stViFrame.stVFrame.u64PTS;

            // mpi_src_fd = RK_MPI_MB_Handle2Fd(stViFrame.stVFrame.pMbBlk);
            // frame_data = (uint8_t*)RK_MPI_MB_Handle2VirAddr(stViFrame.stVFrame.pMbBlk);

            // rga_buffer_handle_t src_handle = importbuffer_fd(mpi_src_fd, stViFrame.stVFrame.u64PrivateData);
            // rga_buffer_t src_rga_buffer = wrapbuffer_handle(src_handle, stViFrame.stVFrame.u32Width, stViFrame.stVFrame.u32Height, RK_FORMAT_YCbCr_420_SP);

            // if (src_handle)
            //     releasebuffer_handle(src_handle);

            // if (stViFrame.stVFrame.u32TimeRef > 20 && stViFrame.stVFrame.u32TimeRef < 25) {
            //     utils_write_file("./main.bin", frame_data, stViFrame.stVFrame.u64PrivateData);
            // }

            s32Ret = RK_MPI_VI_ReleaseChnFrame(vi_chn->ViPipe, vi_chn->viChnId, &stViFrame);
            if (s32Ret != RK_SUCCESS) {
                printf("error: RK_MPI_VI_ReleaseChnFrame fail chn:%d 0x%X\n", vi_chn->viChnId, s32Ret);
            }
        } while (0);
    }
    return RK_NULL;
}

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

    pthread_create(&g_video_main_thread_id, 0, video_main_thread, NULL);

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

    rv1106_video_init_param_t *video_param = get_video_param_list();
    s32Ret = rv1106_video_deinit(video_param);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: rv1106_video_deinit fail: ret:0x%x\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    return s32Ret;
}
