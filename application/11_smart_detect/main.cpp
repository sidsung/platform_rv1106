
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

#include "main.h"

#if CONFIG_ENABLE_ROCKCHIP_YOLOV5
#include "rv1106_yolov5.h"
#elif CONFIG_ENABLE_ROCKCHIP_IVA
#include "im2d.h"
#include "RgaUtils.h"
#include "rv1106_iva.h"
#endif

#include "init_param.h"

#include "graphics_Draw.h"
#include "utils.h"

static bool thread_run = true;

static graphics_image_t g_graphics_image = {0};

int smart_detect_push(char* file_name, int w, int h)
{
    int s32Ret = -1;
    int mmz_buffer_size = 4 * 1024 * 1024;
    smart_detect_result_obj_item_t detect_obj_list = {0};

    MB_BLK mmz_srcBlk = MB_INVALID_HANDLE;
    rga_buffer_handle_t src_handle = 0;

    MB_BLK mmz_dstBlk = MB_INVALID_HANDLE;
    rga_buffer_handle_t dst_handle = 0;

#if CONFIG_ENABLE_ROCKCHIP_YOLOV5
    yolov5_init_param_t *yolov5 = get_yolov5_param();
#elif CONFIG_ENABLE_ROCKCHIP_IVA
    video_iva_param_t *iva = get_iva_param();
    RockIvaImage image = {0};
#endif

    s32Ret = RK_MPI_MMZ_Alloc(&mmz_srcBlk, mmz_buffer_size, RK_MMZ_ALLOC_TYPE_CMA);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] RK_MPI_MMZ_Alloc error: ret:%d\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    s32Ret = RK_MPI_MMZ_Alloc(&mmz_dstBlk, mmz_buffer_size, RK_MMZ_ALLOC_TYPE_CMA);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] RK_MPI_MMZ_Alloc error: ret:%d\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    uint8_t *frame_data = (uint8_t*)RK_MPI_MB_Handle2VirAddr(mmz_srcBlk);

    do {
        s32Ret = utils_read_file(file_name, frame_data, mmz_buffer_size);
        if (s32Ret < 0) {
            printf("[%s %d] utils_read_file error: ret:%d\n", __func__, __LINE__, s32Ret);
            break;
        }

        int mpi_src_fd = RK_MPI_MB_Handle2Fd(mmz_srcBlk);
        src_handle = importbuffer_fd(mpi_src_fd, mmz_buffer_size);
        rga_buffer_t src_rga_buffer = wrapbuffer_handle(src_handle, w, h, RK_FORMAT_YCbCr_420_SP);

        uint8_t *dst_data = (uint8_t*)RK_MPI_MB_Handle2VirAddr(mmz_dstBlk);
        int mpi_dst_fd = RK_MPI_MB_Handle2Fd(mmz_dstBlk);
        dst_handle = importbuffer_fd(mpi_dst_fd, mmz_buffer_size);
        rga_buffer_t rga_dst_img = { 0 };

#if CONFIG_ENABLE_ROCKCHIP_YOLOV5
        s32Ret = rv1106_yolov5_inference(yolov5, src_rga_buffer, 0);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] rv1106_yolov5_inference error: ret:%d\n", __func__, __LINE__, s32Ret);
            // break;
        }

        s32Ret = rv1106_yolov5_get_result(&detect_obj_list);
#elif CONFIG_ENABLE_ROCKCHIP_IVA

        rga_dst_img = wrapbuffer_handle(dst_handle, iva->width, iva->height, RK_FORMAT_YCbCr_420_SP);

        s32Ret = imresize(src_rga_buffer, rga_dst_img);
        if (s32Ret != IM_STATUS_SUCCESS) {
            printf("%d, imresize failed! %s\n", __LINE__, imStrError((IM_STATUS)s32Ret));
            break;
        }

        image.channelId = 0;
        image.dataFd = mpi_dst_fd;
        image.frameId = 1;

        image.info.width = iva->width;
        image.info.height = iva->height;
        image.info.transformMode = ROCKIVA_IMAGE_TRANSFORM_NONE;

        image.info.format = ROCKIVA_IMAGE_FORMAT_YUV420SP_NV12;

        s32Ret = ROCKIVA_PushFrame(iva->handle, &image, NULL);
        if (s32Ret != ROCKIVA_RET_SUCCESS) {
            printf("[%s %d] ROCKIVA_PushFrame error: ret:%d\n", __func__, __LINE__, s32Ret);
            // break;
        }

        usleep(100 * 1000);
        s32Ret = rv1106_iva_get_result(&detect_obj_list);
#endif

        printf("object_number:%d\n", detect_obj_list.object_number);

        if (detect_obj_list.object_number) {
            uint32_t X1, Y1, X2, Y2;

            rga_dst_img = wrapbuffer_handle(dst_handle, w, h, RK_FORMAT_BGR_888);
            s32Ret = imcvtcolor(src_rga_buffer, rga_dst_img, RK_FORMAT_YCbCr_420_SP, RK_FORMAT_BGR_888);
            if (s32Ret != IM_STATUS_SUCCESS) {
                printf("%s running failed, %s\n", "imcvtcolor", imStrError((IM_STATUS)s32Ret));
                break;
            }

            g_graphics_image.width = w;
            g_graphics_image.height = h;
            g_graphics_image.buf = dst_data;

            g_graphics_image.fmt = GD_FMT_BGR888;
            g_graphics_image.line_length = g_graphics_image.width * 3;

            graphics_show_string(&g_graphics_image, 5, 5, file_name, GD_FONT_16x32B, 0xff0000ff, 0);

            if (detect_obj_list.object_number > SMART_DETECT_ITEM_NUM) detect_obj_list.object_number = SMART_DETECT_ITEM_NUM;

            for (int i = 0; i < (int)detect_obj_list.object_number; i++) {
                X1 = ROCKIVA_RATIO_PIXEL_CONVERT(w, detect_obj_list.obj_item[i].x1);
                Y1 = ROCKIVA_RATIO_PIXEL_CONVERT(h, detect_obj_list.obj_item[i].y1);
                X2 = ROCKIVA_RATIO_PIXEL_CONVERT(w, detect_obj_list.obj_item[i].x2);
                Y2 = ROCKIVA_RATIO_PIXEL_CONVERT(h, detect_obj_list.obj_item[i].y2);

                graphics_rectangle(&g_graphics_image, X1, Y1, X2, Y2, 0xff00ff00, 4, 0);

                char info_buf[64];
                snprintf(info_buf, sizeof(info_buf), "%d %s %d%%", i, detect_obj_list.obj_item[i].object_name, detect_obj_list.obj_item[i].score);

                graphics_show_string(&g_graphics_image, X1 + 5, Y1 + 5, info_buf, GD_FONT_16x32B, 0xff0000ff, 0);
            }

            utils_write_file((char*)"output.rgb888", dst_data, w * h * 3);
        }

    } while (0);

    if (src_handle)
        releasebuffer_handle(src_handle);

    if (dst_handle)
        releasebuffer_handle(dst_handle);

    if (mmz_srcBlk != MB_INVALID_HANDLE) {
        RK_MPI_MMZ_Free(mmz_srcBlk);
    }

    if (mmz_dstBlk != MB_INVALID_HANDLE) {
        RK_MPI_MMZ_Free(mmz_dstBlk);
    }

    return s32Ret;
}

static void sigterm_handler(int sig) {
	fprintf(stderr, "signal %d\n", sig);
	thread_run = false;
}

int smart_detect_init()
{
    int s32Ret = -1;
#if CONFIG_ENABLE_ROCKCHIP_YOLOV5
    yolov5_init_param_t *yolov5 = get_yolov5_param();

    s32Ret = rv1106_yolov5_init(yolov5);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: rv1106_yolov5_init s32Ret:0x%X\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }
#elif CONFIG_ENABLE_ROCKCHIP_IVA
    video_iva_param_t *iva = get_iva_param();
    if (iva->enable) {
        s32Ret = rv1106_iva_init(iva);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: rv1106_iva_init s32Ret:0x%X\n", __func__, __LINE__, s32Ret);
            return s32Ret;
        }
    }
#endif
    return s32Ret;
}

int smart_detect_deinit()
{
    int s32Ret = -1;
#if CONFIG_ENABLE_ROCKCHIP_YOLOV5
    yolov5_init_param_t *yolov5 = get_yolov5_param();

    s32Ret = rv1106_yolov5_deinit(yolov5);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: rv1106_yolov5_deinit s32Ret:0x%X\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    printf("rv1106_yolov5_deinit OK\n");

#elif CONFIG_ENABLE_ROCKCHIP_IVA
    video_iva_param_t *iva = get_iva_param();
    if (iva->enable) {
        printf(">>> rv1106_iva_deinit \n");
        s32Ret = rv1106_iva_deinit(iva);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: rv1106_iva_deinit s32Ret:0x%X\n", __func__, __LINE__, s32Ret);
            return s32Ret;
        }
        printf("rv1106_iva_deinit OK\n");
    }

#endif
    return s32Ret;
}

int main(int argc, char *argv[])
{
    int ret = -1;

    signal(SIGINT, sigterm_handler);
    signal(SIGTERM, sigterm_handler);

    do {
        init_video_param_list();
        smart_detect_init();
        smart_detect_push("1280x720.yuv", 1280, 720);

        ret = 0;
    } while (0);
    smart_detect_deinit();

    printf("[%s %d] main exit\n", __FILE__, __LINE__);
    return ret;
}
