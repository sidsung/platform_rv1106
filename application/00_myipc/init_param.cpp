#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

#include "main.h"

#include "rv1106_video_init.h"
#include "rv1106_iva.h"
#include "init_param.h"

static rv1106_video_init_param_t g_video_param_list_ctx;

int g_sensor_raw_width = 1920;
int g_sensor_raw_height = 1080;
int g_sensor_raw_fps = 20;

#if ENABLE_ROCKCHIP_IVA
static video_iva_param_t g_iva_param = { 0 };

video_iva_param_t *get_iva_param(void)
{
    return &g_iva_param;
}
#endif

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
    vi_chn[0].height = 1080;
    vi_chn[0].width = 1920;
    vi_chn[0].SrcFrameRate = -1;
    vi_chn[0].DstFrameRate = -1;
    vi_chn[0].PixelFormat = RK_FMT_YUV420SP;
    vi_chn[0].bMirror = RK_FALSE;
    vi_chn[0].bFlip = RK_FALSE;
    vi_chn[0].bufCount = 2;
    vi_chn[0].Depth = 2;

    // vi_chn[0].Crop = RK_TRUE;
    // vi_chn[0].cropH = g_sensor_raw_height;
    // vi_chn[0].cropW = vi_chn[0].cropH * vi_chn[0].width / vi_chn[0].height;
    // vi_chn[0].cropX = (g_sensor_raw_width - vi_chn[0].cropW) / 2;
    // vi_chn[0].cropY = (g_sensor_raw_height - vi_chn[0].cropH) / 2;

#if ENABLE_ROCKCHIP_IVA
    vi_chn[1].enable = 1;
    vi_chn[1].ViPipe = 0;
    vi_chn[1].viChnId = 1;
    vi_chn[1].width = 576;
    vi_chn[1].height = 324;
    vi_chn[1].SrcFrameRate = g_sensor_raw_fps;
    vi_chn[1].DstFrameRate = 10;
    vi_chn[1].PixelFormat = RK_FMT_YUV420SP;
    vi_chn[1].bMirror = RK_FALSE;
    vi_chn[1].bFlip = RK_FALSE;
    vi_chn[1].bufCount = 2;
    vi_chn[1].Depth = 1;

    // vi_chn[1].Crop = RK_TRUE;
    // vi_chn[1].cropH = g_sensor_raw_height;
    // vi_chn[1].cropW = vi_chn[1].cropH * vi_chn[0].width / vi_chn[0].height;
    // vi_chn[1].cropX = (g_sensor_raw_width - vi_chn[1].cropW) / 2;
    // vi_chn[1].cropY = (g_sensor_raw_height - vi_chn[1].cropH) / 2;
#endif

    vpss[0].enable = 1;
    vpss[0].VpssGrpID = 0;
    vpss[0].inWidth = vi_chn[0].width;
    vpss[0].inHeight = vi_chn[0].height;
    vpss[0].inPixelFormat = RK_FMT_YUV420SP;
    vpss[0].bindSrcChn.enModId = RK_ID_BUTT;
    vpss[0].bindSrcChn.s32DevId = 0;
    vpss[0].bindSrcChn.s32ChnId = 0;

#if ENABLE_SCREEN_PANEL
    vpss[0].chn[0].enable = 1;
    vpss[0].chn[0].VpssChnID = 0;
    vpss[0].chn[0].outWidth = 800;
    vpss[0].chn[0].outHeight = 480;
    vpss[0].chn[0].SrcFrameRate = -1;
    vpss[0].chn[0].DstFrameRate = -1;
    vpss[0].chn[0].outPixelFormat = RK_FMT_YUV420SP;
    vpss[0].chn[0].bMirror = RK_FALSE;
    vpss[0].chn[0].bFlip = RK_FALSE;
    vpss[0].chn[0].bufCount = 2;
    vpss[0].chn[0].Depth = 1;

    // vpss[0].chn[0].Crop = RK_TRUE;
    // vpss[0].chn[0].cropH = vpss[0].inHeight;
    // vpss[0].chn[0].cropW = vpss[0].inHeight * vpss[0].chn[0].outWidth / vpss[0].chn[0].outHeight;
    // vpss[0].chn[0].cropX = (vpss[0].inWidth - vpss[0].chn[0].cropW) / 2;
    // vpss[0].chn[0].cropY = (vpss[0].inHeight - vpss[0].chn[0].cropH) / 2;
#endif

    vpss[0].chn[1].enable = 1;
    vpss[0].chn[1].VpssChnID = 1;
    vpss[0].chn[1].outWidth = vpss[0].inWidth;
    vpss[0].chn[1].outHeight = vpss[0].inHeight;
    vpss[0].chn[1].SrcFrameRate = -1;
    vpss[0].chn[1].DstFrameRate = -1;
    vpss[0].chn[1].outPixelFormat = RK_FMT_YUV420SP;
    vpss[0].chn[1].bMirror = RK_FALSE;
    vpss[0].chn[1].bFlip = RK_FALSE;
    vpss[0].chn[1].bufCount = 2;
    vpss[0].chn[1].Depth = 0;

    venc[0].enable = 1;
    venc[0].vencChnId = 0;
    venc[0].enType = RK_VIDEO_ID_AVC;
    venc[0].bitRate = 5 * 1024;
    venc[0].height = vi_chn[0].height;
    venc[0].width = vi_chn[0].width;
    venc[0].bufSize = venc[0].width * venc[0].width / 2;
    venc[0].bufCount = 1;
    venc[0].PixelFormat = RK_FMT_YUV420SP;
    venc[0].bindSrcChn.enModId = RK_ID_VPSS;
    venc[0].bindSrcChn.s32DevId = 0;
    venc[0].bindSrcChn.s32ChnId = 1;

    venc[1].enable = 0;
    venc[1].vencChnId = 1;
    venc[1].enType = RK_VIDEO_ID_AVC;
    venc[1].bitRate = 2 * 1024;
    venc[1].width = vpss[0].chn[0].outWidth;
    venc[1].height = vpss[0].chn[0].outHeight;
    venc[1].bufSize = venc[1].width * venc[1].width / 2;
    venc[1].bufCount = 1;
    venc[1].PixelFormat = RK_FMT_YUV420SP;
    venc[1].bindSrcChn.enModId = RK_ID_VPSS;
    venc[1].bindSrcChn.s32DevId = 0;
    venc[1].bindSrcChn.s32ChnId = 0;

    rgn[0].enable = 0;
    rgn[0].rgnHandle = 0;
    rgn[0].layer = 0;
    rgn[0].type = OVERLAY_RGN;
    rgn[0].X = 0;
    rgn[0].Y = 0;
    rgn[0].width = vi_chn[0].height;
    rgn[0].height = vi_chn[0].width;

    rgn[0].show = RK_TRUE;
    rgn[0].mppChn.enModId = RK_ID_VENC;
    rgn[0].mppChn.s32DevId = 0;
    rgn[0].mppChn.s32ChnId = 0;
    rgn[0].overlay.format = RK_FMT_BGRA5551;
    rgn[0].overlay.u32FgAlpha = 255;
    rgn[0].overlay.u32BgAlpha = 0;

#if ENABLE_ROCKCHIP_IVA
    g_iva_param.enable = 1;
    g_iva_param.models_path = "/oem/rockiva_data/";
    g_iva_param.width = 576;
    g_iva_param.height = 324;
    g_iva_param.IvaPixelFormat = ROCKIVA_IMAGE_FORMAT_YUV420SP_NV12;
#endif

#if 0
    int i, j;
    i = 0;
    if (vi_chn[i].enable) {
        printf("vi chn %d: w: %d h:%d\n", i, vi_chn[i].width, vi_chn[i].height);
    }

    i = 1;
    if (vi_chn[i].enable) {
        printf("vi chn %d: w: %d h:%d\n", i, vi_chn[i].width, vi_chn[i].height);
    }

    i = 0;
    if (vpss[i].enable) {
        printf("vpss group %d: in w: %d h:%d\n", i, vpss[i].inWidth, vpss[i].inHeight);
        j = 0;
        if (vpss[i].chn[j].enable) {
            printf("vpss group %d chn: %d out w: %d h:%d\n", i, j, vpss[i].chn[j].outWidth, vpss[i].chn[j].outHeight);
            if (vpss[i].chn[j].Crop) {
                printf("vpss group %d chn: %d Crop w: %d h:%d x:%d y:%d\n", i, j, vpss[i].chn[j].cropW, vpss[i].chn[j].cropH, vpss[i].chn[j].cropX, vpss[i].chn[j].cropY);
            }
        }
        j = 1;
        if (vpss[i].chn[j].enable) {
            printf("vpss group %d chn: %d out w: %d h:%d\n", i, j, vpss[i].chn[j].outWidth, vpss[i].chn[j].outHeight);
            if (vpss[i].chn[j].Crop) {
                printf("vpss group %d chn: %d Crop w: %d h:%d x:%d y:%d\n", i, j, vpss[i].chn[j].cropW, vpss[i].chn[j].cropH, vpss[i].chn[j].cropX, vpss[i].chn[j].cropY);
            }
        }
    }

    i = 0;
    if (rgn[i].enable) {
        printf("rgn chn %d: x:%d y:%d w: %d h:%d\n", i, rgn[i].X, rgn[i].Y, rgn[i].width, rgn[i].height);
    }
#endif

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
