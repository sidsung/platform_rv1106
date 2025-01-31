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
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/prctl.h>

#include <errno.h>
#include <pthread.h>
#include <sys/poll.h>

#include "rtsp_demo.h"

#include "main.h"
#include "screen_panel_driver.h"
#include "video.h"

static system_status_t g_system_status;

static bool thread_run = true;

static pthread_t g_system_monitor_thread_id = 0;

#if ENABLE_SCREEN_PANEL
static screen_panel_param_t sp_param = {
    .fb_dev_name = "/dev/fb0",
};

screen_panel_param_t *get_screen_panel_param(void)
{
    return &sp_param;
}
#endif

static void sigterm_handler(int sig) {
	fprintf(stderr, "signal %d\n", sig);
	thread_run = false;
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

system_status_t *get_system_status(void)
{
    return &g_system_status;
}

static void *system_monitor_thread(void *pArgs)
{
    int result;
    FILE *fp = NULL;

    // 设置线程为可取消的
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    // 设置取消类型为延迟取消
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    printf("[%s %d] Start system monitor thread......\n", __FILE__, __LINE__);
    const char *get_cpu_usage = "top -n 1 | grep \"CPU:\" | grep -v \"grep\" | awk '{print $8}'";
    const char *get_npu_usage = "cat /proc/rknpu/load | awk '{print $3}'";
    const char *get_cpu_temp = "cat /sys/class/thermal/thermal_zone0/temp";

    while (thread_run) {
        fp = popen(get_cpu_usage, "r");
        fscanf(fp, "%d", &result);
        pclose(fp);
        g_system_status.cpu_usage = (100 - result) * 10;

        fp = popen(get_cpu_temp, "r");
        fscanf(fp, "%d", &result);
        pclose(fp);
        g_system_status.cpu_temp = result / 100;

        fp = popen(get_npu_usage, "r");
        fscanf(fp, "%d", &result);
        pclose(fp);
        g_system_status.npu_usage = result * 10;

        usleep(1000 * 500);
    }

    return RK_NULL;
}

#if ENABLE_RTSP_PUSH
static pthread_t g_rtsp_stream_thread_id = 0;
static void *rtsp_push_stream_thread(void *pArgs)
{
    int video_ret = -1;
    frameInfo_vi_t fvi_info;

    rtsp_demo_handle rtsplive = NULL;
    rtsp_session_handle rtsp_session;

    fvi_info.frame_data = malloc(1024 * 1024 * 4);
    if (fvi_info.frame_data == NULL) {
        fprintf(stderr, "[%s %d] malloc err\n", __FILE__, __LINE__);
        return RK_NULL;
    }

    // init rtsp
    rtsplive = create_rtsp_demo(554);

    const char* rtsp_path = "/live/0";
    printf("[%s %d] create rtsp server: RTSP://IP:554/%s\n", __FILE__, __LINE__, rtsp_path);
    rtsp_session = rtsp_new_session(rtsplive, rtsp_path);
    rtsp_set_video(rtsp_session, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
    rtsp_sync_video_ts(rtsp_session, rtsp_get_reltime(), rtsp_get_ntptime());

    // const char* rtsp_path2 = "/live/1";
    // printf("[%s %d] create rtsp server: RTSP://IP:554/%s\n", __FILE__, __LINE__, rtsp_path2);
    // rtsp_session_handle rtsp_session2 = rtsp_new_session(rtsplive, rtsp_path2);
    // rtsp_set_video(rtsp_session2, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
    // rtsp_sync_video_ts(rtsp_session2, rtsp_get_reltime(), rtsp_get_ntptime());

    printf("[%s %d] Start Rtsp push stream thread......\n", __FILE__, __LINE__);
    while (thread_run) {
        video_ret = video_GetFrame(GET_VENC_FRAME, &fvi_info, NULL);
        if (!video_ret) {
            rtsp_tx_video(rtsp_session, fvi_info.frame_data, fvi_info.frame_size,
                            fvi_info.timestamp);
            rtsp_do_event(rtsplive);
        }

        // video_ret = video_GetFrame(GET_VENC2_FRAME, &fvi_info, NULL);
        // if (!video_ret) {
        //     rtsp_tx_video(rtsp_session2, fvi_info.frame_data, fvi_info.frame_size,
        //                     fvi_info.timestamp);
        //     rtsp_do_event(rtsplive);
        // }

        // static uint64_t last_timestamp = 0;
        // printf("RTSP ---> seq:%d w:%d h:%d fmt:%d size:%lld delay:%dms fps:%.1f\n", fvi_info.frame_seq, fvi_info.width, fvi_info.height, fvi_info.PixelFormat, fvi_info.frame_size, (uint32_t)(fvi_info.timestamp - last_timestamp) / 1000, (1000.0 / ((fvi_info.timestamp - last_timestamp) / 1000)));
        // last_timestamp = fvi_info.timestamp;
    }
    if (fvi_info.frame_data) {
        free(fvi_info.frame_data);
    }

    if (rtsplive)
        rtsp_del_demo(rtsplive);

    return RK_NULL;
}
#endif

int main(int argc, char *argv[])
{
    int ret = -1;

    signal(SIGINT, sigterm_handler);
    signal(SIGTERM, sigterm_handler);

#if ENABLE_SCREEN_PANEL
    if (screen_panel_init(&sp_param)) {
        printf("ERROR: screen_panel_init faile, exit\n");
        return -1;
    }
#endif

    do {
        if (video_init()) {
            printf("ERROR: video_init faile, exit\n");
            break;
        }

        pthread_create(&g_system_monitor_thread_id, 0, system_monitor_thread, NULL);

#if ENABLE_RTSP_PUSH
        pthread_create(&g_rtsp_stream_thread_id, 0, rtsp_push_stream_thread, NULL);
#endif

        while (thread_run) {
            sleep(1);
        }
        printf("%s exit!\n", __func__);

        if (g_system_monitor_thread_id) {
            pthread_cancel(g_system_monitor_thread_id);
            printf("[%s %d] wait system monitor thread join\n", __FILE__, __LINE__);
            pthread_join(g_system_monitor_thread_id, NULL);
        }

#if ENABLE_RTSP_PUSH
        if (g_rtsp_stream_thread_id) {
            printf("[%s %d] wait rtsp thread join\n", __FILE__, __LINE__);
            pthread_join(g_rtsp_stream_thread_id, NULL);
        }
#endif

        ret = 0;
    } while (0);

    printf("[%s %d] video_deinit \n", __FILE__, __LINE__);
    video_deinit();

#if ENABLE_SCREEN_PANEL
    screen_panel_deinit(&sp_param);
#endif

    printf("[%s %d] main exit\n", __FILE__, __LINE__);
    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
