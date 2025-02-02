#ifndef _RV1106_YOLOV5_H_
#define _RV1106_YOLOV5_H_

#include "rv1106_common.h"

#include "im2d.h"
#include "RgaUtils.h"

#include "rknn_app.h"

typedef struct {
    const char *model_path;
    const char *labels_file;

    rknn_app_ctx_t rknn_ctx;
} yolov5_init_param_t;

int rv1106_yolov5_init(yolov5_init_param_t *yolo);
int rv1106_yolov5_deinit(yolov5_init_param_t *yolo);

int rv1106_yolov5_inference(yolov5_init_param_t *yolo, rga_buffer_t src_rga_buffer, uint32_t frameId);

int rv1106_yolov5_get_result(smart_detect_result_obj_item_t *item);

#endif
