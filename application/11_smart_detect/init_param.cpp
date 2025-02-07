#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

#include "main.h"

int g_sensor_raw_width = 1920;
int g_sensor_raw_height = 1080;
int g_sensor_raw_fps = 20;

#if CONFIG_ENABLE_ROCKCHIP_YOLOV5
#include "rv1106_yolov5.h"

static yolov5_init_param_t g_yolov5_param = { 0 };
yolov5_init_param_t *get_yolov5_param(void)
{
    return &g_yolov5_param;
}
#elif CONFIG_ENABLE_ROCKCHIP_IVA
#include "rv1106_iva.h"

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
    init_get_sensor_raw_size();

#if CONFIG_ENABLE_ROCKCHIP_YOLOV5
    g_yolov5_param.model_path = "/oem/rknn_model/yolov5.rknn";
    g_yolov5_param.labels_file = "/oem/rknn_model/coco_80_labels_list.txt";
#elif CONFIG_ENABLE_ROCKCHIP_IVA
    g_iva_param.enable = 1;
    g_iva_param.models_path = "/oem/rockiva_data/";
    g_iva_param.width = 576;
    g_iva_param.height = 324;
    g_iva_param.IvaPixelFormat = ROCKIVA_IMAGE_FORMAT_YUV420SP_NV12;
#endif

}
