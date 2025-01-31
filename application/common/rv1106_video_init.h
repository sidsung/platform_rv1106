#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */
#ifndef _RV1106_VIDEO_INIT_H_
#define _RV1106_VIDEO_INIT_H_

#include "rv1106_common.h"

#include "rv1106_isp.h"
#include "rv1106_vi.h"
#include "rv1106_vpss.h"
#include "rv1106_venc.h"
#include "rv1106_rgn.h"

#define SMART_DETECT_ITEM_NUM 16

typedef struct {
    uint32_t type_index;              /* 目标类型 */
    uint32_t score;                  /* 目标检测分数 [1-100] */
    char object_name[32];
    uint32_t name_color;
    uint16_t x1;                      /* 万分比表示，数值范围0~9999 */
    uint16_t y1;                      /* 万分比表示，数值范围0~9999 */
    uint16_t x2;                      /* 万分比表示，数值范围0~9999 */
    uint16_t y2;                      /* 万分比表示，数值范围0~9999 */
    uint32_t rect_color;
} smart_detect_result_obj_t;

typedef struct {
    int object_number;
    uint32_t frameId;          /* 所在帧序号 */
    smart_detect_result_obj_t obj_item[SMART_DETECT_ITEM_NUM];
} smart_detect_result_obj_item_t;

typedef struct {
    video_isp_param_t isp[1];
    video_vi_dev_param_t vi_dev[1];
    video_vi_chn_param_t vi_chn[6];
    video_vpss_param_t vpss[1];
    video_venc_param_t venc[2];
    video_rgn_param_t rgn[8];
} rv1106_video_init_param_t;

int rv1106_video_init(rv1106_video_init_param_t *video_param);
int rv1106_video_deinit(rv1106_video_init_param_t *video_param);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
