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
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/prctl.h>

#include <errno.h>
#include <pthread.h>
#include <sys/poll.h>

#include "video.h"

#include "main.h"

#include "rtsp_demo.h"

static bool thread_quit = false;


static pthread_t g_venc_stream_thread_id = 0;

static void sigterm_handler(int sig) {
	fprintf(stderr, "signal %d\n", sig);
	thread_quit = true;
}

void show_hex(uint8_t *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        printf("%x ", buf[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}

static void *venc_stream_thread(void *pArgs)
{
    int video_ret = -1;
    frameInfo_vi_t fvi_info;
    rtsp_session_handle rtsp_session0;
    rtsp_demo_handle rtsplive = NULL;

    fvi_info.frame_data = malloc(1024 * 1024 * 8);
    if (fvi_info.frame_data == NULL) {
        fprintf(stderr, "[%s %d] malloc err\n", __FILE__, __LINE__);
        return RK_NULL;
    }

    // init rtsp
    const char* rtsp_path0 = "/live/0";
    rtsplive = create_rtsp_demo(554);
    printf("[%s %d] create rtsp server: RTSP://IP:554/%s\n", __func__, __LINE__, rtsp_path0);
    rtsp_session0 = rtsp_new_session(rtsplive, rtsp_path0);

    rtsp_set_video(rtsp_session0, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
    rtsp_sync_video_ts(rtsp_session0, rtsp_get_reltime(), rtsp_get_ntptime());

    printf("[%s %d] Start VENC stream thread......\n", __FILE__, __LINE__);
    while (!thread_quit) {
        video_ret = video_GetFrame(GET_VENC_FRAME, &fvi_info, NULL);
        if (!video_ret) {
            // static uint64_t last_timestamp = 0;
            // printf("VENC ---> seq:%d w:%d h:%d fmt:%d size:%lld delay:%dms fps:%.1f\n", fvi_info.frame_seq, fvi_info.width, fvi_info.height, fvi_info.PixelFormat, fvi_info.frame_size, (uint32_t)(fvi_info.timestamp - last_timestamp) / 1000, (1000.0 / ((fvi_info.timestamp - last_timestamp) / 1000)));
            // last_timestamp = fvi_info.timestamp;

            rtsp_tx_video(rtsp_session0, fvi_info.frame_data, fvi_info.frame_size,
                            fvi_info.timestamp);
            rtsp_do_event(rtsplive);
            ;
        }
        usleep(20 * 1000);
    }

    if (rtsplive)
        rtsp_del_demo(rtsplive);

    if (fvi_info.frame_data) {
        free(fvi_info.frame_data);
    }
    return RK_NULL;
}

int main(int argc, char *argv[])
{
    int ret = -1;

    signal(SIGINT, sigterm_handler);
    signal(SIGTERM, sigterm_handler);

    do {
        if (video_init()) {
            printf("ERROR: video_init faile, exit\n");
            break;
        }

        pthread_create(&g_venc_stream_thread_id, 0, venc_stream_thread, NULL);

        while (!thread_quit) {
            sleep(1);
        }
        printf("%s exit!\n", __func__);

        if (g_venc_stream_thread_id) {
            printf("[%s %d] wait rtsp thread join\n", __FILE__, __LINE__);
            pthread_join(g_venc_stream_thread_id, NULL);
        }

        ret = 0;
    } while (0);

    printf("[%s %d] video_deinit \n", __FILE__, __LINE__);
    video_deinit();

    printf("[%s %d] main exit\n", __FILE__, __LINE__);
    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
