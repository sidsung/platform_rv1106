#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

#include "rv1106_video_init.h"
#include "init_param.h"

static rv1106_video_init_param_t g_video_param_list_ctx;

int g_sensor_raw_width = 1920;
int g_sensor_raw_height = 1080;
int g_sensor_raw_fps = 20;

static int init_get_sensor_raw_size(void)
{
    FILE *fp = NULL;
    char buf[256] = {0};
    char *p = NULL;

    fp = popen("cat /proc/rkisp-vir0 | grep Input | awk -F '[:]' '{print $3}'", "r");
    if (fp == NULL) {
        printf("error: cat /proc/rkisp-vir0 failed\n");
        return -1;
    }

    if (fgets(buf, sizeof(buf), fp) == NULL) {
        printf("error: read /proc/rkisp-vir0 failed\n");
        pclose(fp);
        return -1;
    }

    pclose(fp);

    p = strtok(buf, "x");
    if (p) {
        g_sensor_raw_width = atoi(p);
    }

    p = strtok(NULL, "@");
    if (p) {
        g_sensor_raw_height = atoi(p);
    }

    p = strtok(NULL, "@");
    if (p) {
        g_sensor_raw_fps = atoi(p);
    }

    printf("sensor raw size: %dx%d fps:%d\n", g_sensor_raw_width, g_sensor_raw_height, g_sensor_raw_fps);

    return 0;
}

void init_video_param_list(void)
{
    memset(&g_video_param_list_ctx, 0, sizeof(rv1106_video_init_param_t));

    init_get_sensor_raw_size();

    video_vi_chn_param_t *vi_chn = g_video_param_list_ctx.vi_chn;
    video_vpss_param_t *vpss = g_video_param_list_ctx.vpss;
    video_venc_param_t *venc = g_video_param_list_ctx.venc;
    video_rgn_param_t *rgn = g_video_param_list_ctx.rgn;

    g_video_param_list_ctx.isp[0].enable = 1;
    g_video_param_list_ctx.isp[0].ViDevId = 0;
    g_video_param_list_ctx.isp[0].iq_file_dir = "/etc/iqfiles";
    g_video_param_list_ctx.isp[0].hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;

    g_video_param_list_ctx.vi_dev[0].enable = 1;
    g_video_param_list_ctx.vi_dev[0].ViDevId = 0;
    g_video_param_list_ctx.vi_dev[0].BindPipe.u32Num = 1;
    g_video_param_list_ctx.vi_dev[0].BindPipe.PipeId[0] = 0;

    vi_chn[0].enable = 1;
    vi_chn[0].ViPipe = 0;
    vi_chn[0].viChnId = 0;
    vi_chn[0].height = g_sensor_raw_height;
    vi_chn[0].width = g_sensor_raw_width;
    vi_chn[0].SrcFrameRate = -1;
    vi_chn[0].DstFrameRate = -1;
    vi_chn[0].PixelFormat = RK_FMT_YUV420SP;
    vi_chn[0].bMirror = RK_FALSE;
    vi_chn[0].bFlip = RK_FALSE;
    vi_chn[0].bufCount = 2;
    vi_chn[0].Depth = 2;
}

rv1106_video_init_param_t *get_video_param_list(void)
{
    return &g_video_param_list_ctx;
}

video_isp_param_t *get_isp_param(void)
{
    return &g_video_param_list_ctx.isp[0];
}

video_vi_dev_param_t *get_vi_dev_param(void)
{
    return &g_video_param_list_ctx.vi_dev[0];
}

video_vi_chn_param_t *get_vi_chn_param(void)
{
    return &g_video_param_list_ctx.vi_chn[0];
}

video_vpss_param_t *get_vpss_param(void)
{
    return &g_video_param_list_ctx.vpss[0];
}

video_venc_param_t *get_venc_param(void)
{
    return &g_video_param_list_ctx.venc[0];
}

video_rgn_param_t *get_rgn_param(void)
{
    return &g_video_param_list_ctx.rgn[0];
}
