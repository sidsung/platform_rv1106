#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

#include "video.h"
#include "rv1106_video_init.h"
#include "init_param.h"

static rv1106_video_init_param_t g_video_param_list_ctx;

int g_sensor_raw_width = 1920;
int g_sensor_raw_height = 1080;

static int init_get_sensor_raw_size(void)
{
    FILE *fp = NULL;
    char buf[256] = {0};
    char *p = NULL;

    fp = popen("cat /proc/rkisp-vir0 | grep Input | awk -F '[:@]' '{print $3}'", "r");
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

    p = strtok(NULL, "x");
    if (p) {
        g_sensor_raw_height = atoi(p);
    }

    printf("sensor raw size: %dx%d\n", g_sensor_raw_width, g_sensor_raw_height);

    return 0;
}


void init_video_param_list(void)
{
    memset(&g_video_param_list_ctx, 0, sizeof(rv1106_video_init_param_t));

    init_get_sensor_raw_size();

    g_video_param_list_ctx.isp[0].enable = 1;
    g_video_param_list_ctx.isp[0].ViDevId = 0;
    g_video_param_list_ctx.isp[0].iq_file_dir = "/etc/iqfiles";
    g_video_param_list_ctx.isp[0].hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;

    g_video_param_list_ctx.vi_dev[0].enable = 1;
    g_video_param_list_ctx.vi_dev[0].ViDevId = 0;
    g_video_param_list_ctx.vi_dev[0].BindPipe.u32Num = 1;
    g_video_param_list_ctx.vi_dev[0].BindPipe.PipeId[0] = 0;

    g_video_param_list_ctx.vi_chn[0].enable = 1;
    g_video_param_list_ctx.vi_chn[0].ViPipe = 0;
    g_video_param_list_ctx.vi_chn[0].viChnId = 0;
    g_video_param_list_ctx.vi_chn[0].height = 1080;
    g_video_param_list_ctx.vi_chn[0].width = g_video_param_list_ctx.vi_chn[0].height * g_sensor_raw_width / g_sensor_raw_height;
    g_video_param_list_ctx.vi_chn[0].SrcFrameRate = -1;
    g_video_param_list_ctx.vi_chn[0].DstFrameRate = -1;
    g_video_param_list_ctx.vi_chn[0].PixelFormat = RK_FMT_YUV420SP;
    g_video_param_list_ctx.vi_chn[0].bMirror = RK_FALSE;
    g_video_param_list_ctx.vi_chn[0].bFlip = RK_FALSE;
    g_video_param_list_ctx.vi_chn[0].bufCount = 2;
    g_video_param_list_ctx.vi_chn[0].Depth = 0;

    g_video_param_list_ctx.vi_chn[1].enable = 0;
    g_video_param_list_ctx.vi_chn[1].ViPipe = 0;
    g_video_param_list_ctx.vi_chn[1].viChnId = 1;
    g_video_param_list_ctx.vi_chn[1].width = 1280;
    g_video_param_list_ctx.vi_chn[1].height = 720;
    g_video_param_list_ctx.vi_chn[1].SrcFrameRate = -1;
    g_video_param_list_ctx.vi_chn[1].DstFrameRate = -1;
    g_video_param_list_ctx.vi_chn[1].PixelFormat = RK_FMT_YUV420SP;
    g_video_param_list_ctx.vi_chn[1].bMirror = RK_FALSE;
    g_video_param_list_ctx.vi_chn[1].bFlip = RK_FALSE;
    g_video_param_list_ctx.vi_chn[1].bufCount = 2;
    g_video_param_list_ctx.vi_chn[1].Depth = 0;

    g_video_param_list_ctx.vi_chn[2].enable = 0;
    g_video_param_list_ctx.vi_chn[2].ViPipe = 0;
    g_video_param_list_ctx.vi_chn[2].viChnId = 2;
    g_video_param_list_ctx.vi_chn[2].width = 576;
    g_video_param_list_ctx.vi_chn[2].height = 324;
    g_video_param_list_ctx.vi_chn[2].SrcFrameRate = 25;
    g_video_param_list_ctx.vi_chn[2].DstFrameRate = 10;
    g_video_param_list_ctx.vi_chn[2].PixelFormat = RK_FMT_YUV420SP;
    g_video_param_list_ctx.vi_chn[2].bMirror = RK_FALSE;
    g_video_param_list_ctx.vi_chn[2].bFlip = RK_FALSE;
    g_video_param_list_ctx.vi_chn[2].bufCount = 3;
    g_video_param_list_ctx.vi_chn[2].Depth = 1;

    g_video_param_list_ctx.vpss[0].enable = 0;
    g_video_param_list_ctx.vpss[0].VpssGrpID = 0;
    g_video_param_list_ctx.vpss[0].inWidth = g_sensor_raw_width;
    g_video_param_list_ctx.vpss[0].inHeight = g_sensor_raw_height;
    g_video_param_list_ctx.vpss[0].inPixelFormat = RK_FMT_YUV420SP;
    g_video_param_list_ctx.vpss[0].bindSrcChn.enModId = RK_ID_VI;
    g_video_param_list_ctx.vpss[0].bindSrcChn.s32DevId = 0;
    g_video_param_list_ctx.vpss[0].bindSrcChn.s32ChnId = 0;

    g_video_param_list_ctx.vpss[0].chn[0].enable = 0;
    g_video_param_list_ctx.vpss[0].chn[0].VpssChnID = 0;
    g_video_param_list_ctx.vpss[0].chn[0].outWidth = g_sensor_raw_width;
    g_video_param_list_ctx.vpss[0].chn[0].outHeight = g_sensor_raw_height;
    g_video_param_list_ctx.vpss[0].chn[0].SrcFrameRate = -1;
    g_video_param_list_ctx.vpss[0].chn[0].DstFrameRate = -1;
    g_video_param_list_ctx.vpss[0].chn[0].outPixelFormat = RK_FMT_YUV420SP;
    g_video_param_list_ctx.vpss[0].chn[0].bMirror = RK_FALSE;
    g_video_param_list_ctx.vpss[0].chn[0].bFlip = RK_FALSE;
    g_video_param_list_ctx.vpss[0].chn[0].bufCount = 2;
    g_video_param_list_ctx.vpss[0].chn[0].Depth = 0;

    g_video_param_list_ctx.vpss[0].chn[1].enable = 1;
    g_video_param_list_ctx.vpss[0].chn[1].VpssChnID = 1;
    g_video_param_list_ctx.vpss[0].chn[1].outWidth = 1280;
    g_video_param_list_ctx.vpss[0].chn[1].outHeight = 720;
    g_video_param_list_ctx.vpss[0].chn[1].SrcFrameRate = -1;
    g_video_param_list_ctx.vpss[0].chn[1].DstFrameRate = -1;
    g_video_param_list_ctx.vpss[0].chn[1].outPixelFormat = RK_FMT_YUV420SP;
    g_video_param_list_ctx.vpss[0].chn[1].bMirror = RK_FALSE;
    g_video_param_list_ctx.vpss[0].chn[1].bFlip = RK_FALSE;
    g_video_param_list_ctx.vpss[0].chn[1].bufCount = 2;
    g_video_param_list_ctx.vpss[0].chn[1].Depth = 0;

    g_video_param_list_ctx.venc[0].enable = 1;
    g_video_param_list_ctx.venc[0].vencChnId = 0;
    g_video_param_list_ctx.venc[0].enType = RK_VIDEO_ID_AVC;
    g_video_param_list_ctx.venc[0].bitRate = 2 * 1024;
    g_video_param_list_ctx.venc[0].height = 1080;
    g_video_param_list_ctx.venc[0].width = g_video_param_list_ctx.venc[0].height * g_sensor_raw_width / g_sensor_raw_height;
    g_video_param_list_ctx.venc[0].bufSize = g_video_param_list_ctx.venc[0].width * g_video_param_list_ctx.venc[0].width / 2;
    g_video_param_list_ctx.venc[0].bufCount = 1;
    g_video_param_list_ctx.venc[0].PixelFormat = RK_FMT_YUV420SP;
    g_video_param_list_ctx.venc[0].bindSrcChn.enModId = RK_ID_VI;
    g_video_param_list_ctx.venc[0].bindSrcChn.s32DevId = 0;
    g_video_param_list_ctx.venc[0].bindSrcChn.s32ChnId = 0;

    g_video_param_list_ctx.venc[1].enable = 0;
    g_video_param_list_ctx.venc[1].vencChnId = 1;
    g_video_param_list_ctx.venc[1].enType = RK_VIDEO_ID_AVC;
    g_video_param_list_ctx.venc[1].bitRate = 2 * 1024;
    g_video_param_list_ctx.venc[1].height = 720;
    g_video_param_list_ctx.venc[1].width = g_video_param_list_ctx.venc[0].height * g_sensor_raw_width / g_sensor_raw_height;
    g_video_param_list_ctx.venc[1].bufSize = g_video_param_list_ctx.venc[1].width * g_video_param_list_ctx.venc[1].width / 2;
    g_video_param_list_ctx.venc[1].bufCount = 1;
    g_video_param_list_ctx.venc[1].PixelFormat = RK_FMT_YUV420SP;
    g_video_param_list_ctx.venc[1].bindSrcChn.enModId = RK_ID_VPSS;
    g_video_param_list_ctx.venc[1].bindSrcChn.s32DevId = 0;
    g_video_param_list_ctx.venc[1].bindSrcChn.s32ChnId = 1;

    g_video_param_list_ctx.rgn[0].enable = 0;
    g_video_param_list_ctx.rgn[0].rgnHandle = 0;
    g_video_param_list_ctx.rgn[0].layer = 0;
    g_video_param_list_ctx.rgn[0].type = OVERLAY_RGN;
    g_video_param_list_ctx.rgn[0].X = 0;
    g_video_param_list_ctx.rgn[0].Y = 0;
    g_video_param_list_ctx.rgn[0].width = g_sensor_raw_width;
    g_video_param_list_ctx.rgn[0].height = g_sensor_raw_height;
    g_video_param_list_ctx.rgn[0].show = RK_TRUE;
    g_video_param_list_ctx.rgn[0].mppChn.enModId = RK_ID_VENC;
    g_video_param_list_ctx.rgn[0].mppChn.s32DevId = 0;
    g_video_param_list_ctx.rgn[0].mppChn.s32ChnId = 0;
    g_video_param_list_ctx.rgn[0].overlay.format = RK_FMT_BGRA5551;
    g_video_param_list_ctx.rgn[0].overlay.u32FgAlpha = 255;
    g_video_param_list_ctx.rgn[0].overlay.u32BgAlpha = 0;

    g_video_param_list_ctx.iva[0].enable = 0;
    g_video_param_list_ctx.iva[0].models_path = "/oem/rockiva_data/";
    g_video_param_list_ctx.iva[0].width = 576;
    g_video_param_list_ctx.iva[0].height = 324;
    g_video_param_list_ctx.iva[0].IvaPixelFormat = ROCKIVA_IMAGE_FORMAT_YUV420SP_NV12;
    // g_video_param_list_ctx.iva[0].result_cb = rv1106_iva_result_cb;

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

video_iva_param_t *get_iva_param(void)
{
    return &g_video_param_list_ctx.iva[0];
}
