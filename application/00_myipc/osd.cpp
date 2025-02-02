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

#include "dma_alloc.h"

#include "im2d.h"
#include "RgaUtils.h"

#include "graphics_Draw.h"
#include "video.h"

#include "osd.h"
#include "utils.h"

#include "main.h"
#include "rv1106_video_init.h"

#if CONFIG_ENABLE_ROCKCHIP_IVA
#include "rv1106_iva.h"
#endif

#if CONFIG_ENABLE_ROCKCHIP_YOLOV5
#include "rv1106_yolov5.h"
#endif

static graphics_image_t g_graphics_image = {0};

static char *get_time_str(void)
{
    static char str_buf[64];
    time_t current_time;
    struct tm *local_time;
    time(&current_time);  // 获取当前时间
    local_time = localtime(&current_time);  // 转换为本地时间

    snprintf(str_buf, sizeof(str_buf), "%04d-%02d-%02d %02d:%02d:%02d",
           local_time->tm_year + 1900,  // 年份（从1900年开始）
           local_time->tm_mon + 1,      // 月份（0-11，需要加1）
           local_time->tm_mday,         // 日
           local_time->tm_hour,         // 时
           local_time->tm_min,          // 分
           local_time->tm_sec);         // 秒
    return str_buf;
}

static im_osd_t g_osd_config = {
    .osd_mode = IM_OSD_MODE_STATISTICS | IM_OSD_MODE_AUTO_INVERT,
    .block_parm = {
        .width_mode = IM_ALPHA_COLORKEY_NORMAL,
        .width = 0,
        .block_count = 0,
        .background_config = IM_OSD_BACKGROUND_DEFAULT_BRIGHT,
        .direction = IM_OSD_MODE_HORIZONTAL,
        .color_mode = IM_OSD_COLOR_PIXEL,
    },
    .invert_config = {
        .invert_channel = IM_OSD_INVERT_CHANNEL_NONE,
        .flags_mode = IM_OSD_FLAGS_INTERNAL,
        .flags_index = 1,
        .invert_flags = 0x000000000000002a,
        .invert_mode = IM_OSD_INVERT_USE_SWAP,
        .threash = 40,
    },
};

static int update_osd_draw_smart_detect(VIDEO_FRAME_INFO_S *stViFrame)
{
    int s32Ret = 0;
    smart_detect_result_obj_item_t detect_obj_list = {0};

#if CONFIG_ENABLE_ROCKCHIP_IVA
    s32Ret = rv1106_iva_get_result(&detect_obj_list);
#endif

#if CONFIG_ENABLE_ROCKCHIP_YOLOV5
    s32Ret = rv1106_yolov5_get_result(&detect_obj_list);
#endif

    if (detect_obj_list.object_number) {
        uint32_t X1, Y1, X2, Y2;

        if (detect_obj_list.object_number > SMART_DETECT_ITEM_NUM) detect_obj_list.object_number = SMART_DETECT_ITEM_NUM;

        for (int i = 0; i < (int)detect_obj_list.object_number; i++) {
            X1 = ROCKIVA_RATIO_PIXEL_CONVERT(stViFrame->stVFrame.u32Width, detect_obj_list.obj_item[i].x1);
            Y1 = ROCKIVA_RATIO_PIXEL_CONVERT(stViFrame->stVFrame.u32Height, detect_obj_list.obj_item[i].y1);
            X2 = ROCKIVA_RATIO_PIXEL_CONVERT(stViFrame->stVFrame.u32Width, detect_obj_list.obj_item[i].x2);
            Y2 = ROCKIVA_RATIO_PIXEL_CONVERT(stViFrame->stVFrame.u32Height, detect_obj_list.obj_item[i].y2);

            graphics_rectangle(&g_graphics_image, X1, Y1, X2, Y2, color_convert_argb4444_le(detect_obj_list.obj_item[i].rect_color >> 24, ((detect_obj_list.obj_item[i].rect_color & 0x00ff0000) >> 16), ((detect_obj_list.obj_item[i].rect_color & 0x0000ff00) >> 8), (detect_obj_list.obj_item[i].rect_color & 0x000000ff)), 4, 0);

            graphics_show_string(&g_graphics_image, X1 + 5, Y1 + 5, detect_obj_list.obj_item[i].object_name, GD_FONT_16x32B, color_convert_argb4444_le(detect_obj_list.obj_item[i].name_color >> 24, ((detect_obj_list.obj_item[i].name_color & 0x00ff0000) >> 16), ((detect_obj_list.obj_item[i].name_color & 0x0000ff00) >> 8), (detect_obj_list.obj_item[i].name_color & 0x000000ff)), 0);
        }
    }

    return s32Ret;
}

int update_osd(VIDEO_FRAME_INFO_S *stViFrame, rga_buffer_t src_rga_buffer, uint8_t* frame_data)
{
    int s32Ret = -1;

    char *fg_buf = NULL;
    int fg_dma_fd;
    rga_buffer_handle_t fg_handle;
    rga_buffer_t fg_img;

    im_rect bg_rect;

    int fg_width = stViFrame->stVFrame.u32Width;
    int fg_height = stViFrame->stVFrame.u32Height;
    int fg_format = RK_FORMAT_ARGB_4444;

    int fg_buf_size = fg_width * fg_height * get_bpp_from_format(fg_format);

    s32Ret = dma_buf_alloc(RV1106_CMA_HEAP_PATH, fg_buf_size, &fg_dma_fd, (void **)&fg_buf);
    if (s32Ret < 0) {
        printf("alloc fg CMA buffer failed!\n");
        fg_buf = NULL;
        return s32Ret;
    }

    static uint64_t last_timestamp = 0;
    float fps = (1000.0 / ((stViFrame->stVFrame.u64PTS - last_timestamp) / 1000));
    last_timestamp = stViFrame->stVFrame.u64PTS;

    g_graphics_image.width = fg_width;
    g_graphics_image.height = fg_height;
    g_graphics_image.buf = (uint8_t*)(fg_buf);

    g_graphics_image.fmt = GD_FMT_RAW_2B;
    g_graphics_image.line_length = g_graphics_image.width * 2;

    char *time = get_time_str();
    graphics_show_string(&g_graphics_image, 0, 0, time, GD_FONT_16x32B, color_convert_argb4444_le(255, 255, 255, 0), 0);

    char info_buf[64];
    system_status_t *sys_info = get_system_status();
    snprintf(info_buf, sizeof(info_buf), "CPU: %d%% %d'C / NPU: %d%%", (int)(sys_info->cpu_usage / 10), (int)(sys_info->cpu_temp / 10), (int)(sys_info->npu_usage / 10));
    graphics_show_string(&g_graphics_image, 0, 32, info_buf, GD_FONT_16x32B, color_convert_argb4444_le(255, 255, 255, 0), 0);

    snprintf(info_buf, sizeof(info_buf), "seq: %d fps: %.1f", stViFrame->stVFrame.u32TimeRef, fps);
    graphics_show_string(&g_graphics_image, 0, 64, info_buf, GD_FONT_16x32B, color_convert_argb4444_le(255, 255, 255, 0), 0);

    update_osd_draw_smart_detect(stViFrame);

    fg_handle = importbuffer_fd(fg_dma_fd, fg_buf_size);
    if (fg_handle == 0) {
        printf("importbuffer failed!\n");
        dma_buf_free(fg_buf_size, &fg_dma_fd, fg_buf);
        fg_buf = NULL;
        s32Ret = -1;
        return s32Ret;
    }

    fg_img = wrapbuffer_handle(fg_handle, fg_width, fg_height, fg_format);

    bg_rect.x = 0;
    bg_rect.y = 0;
    bg_rect.width = fg_width;
    bg_rect.height = fg_height;
/*
    s32Ret = imcheck(fg_img, src_rga_buffer, {}, bg_rect);
    if (IM_STATUS_NOERROR != s32Ret) {
        printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)s32Ret));
        s32Ret = -1;
    }
*/
    g_osd_config.block_parm.width = fg_width;
    g_osd_config.block_parm.block_count = 1;

    s32Ret = imosd(fg_img, src_rga_buffer, bg_rect, &g_osd_config);
    if (s32Ret != IM_STATUS_SUCCESS) {
        printf("%d imosd running failed, %s\n", __LINE__, imStrError((IM_STATUS)s32Ret));
        s32Ret = -1;
    }

    if (fg_handle)
        releasebuffer_handle(fg_handle);

    if (fg_buf)
        dma_buf_free(fg_buf_size, &fg_dma_fd, fg_buf);

    return s32Ret;
}
