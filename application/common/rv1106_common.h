#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef _RV1106_COMMON_H_
#define _RV1106_COMMON_H_

#include "rk_debug.h"
#include "rk_defines.h"
#include "rk_mpi_adec.h"
#include "rk_mpi_aenc.h"
#include "rk_mpi_ai.h"
#include "rk_mpi_ao.h"
#include "rk_mpi_avs.h"
#include "rk_mpi_cal.h"
#include "rk_mpi_ivs.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_rgn.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_tde.h"
#include "rk_mpi_vdec.h"
#include "rk_mpi_venc.h"
#include "rk_mpi_vi.h"
#include "rk_mpi_vo.h"
#include "rk_mpi_vpss.h"
#include "rk_mpi_mmz.h"

/*     ROCKIVA   */
#include "rockiva/rockiva_ba_api.h"

#include "rockiva/rockiva_common.h"
#include "rockiva/rockiva_det_api.h"
#include "rockiva/rockiva_face_api.h"
#include "rockiva/rockiva_image.h"

/*     RKAIQ   */
#include "rk_aiq_comm.h"

#include "rv1106_gpio.h"

#define RK_ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#define RK_ALIGN_2(x) RK_ALIGN(x, 2)
#define RK_ALIGN_16(x) RK_ALIGN(x, 16)
#define RK_ALIGN_32(x) RK_ALIGN(x, 32)

#define SMART_DETECT_ITEM_NUM 64

typedef struct {
    int width;
    int height;
    uint32_t frame_seq;
    uint64_t timestamp;
    uint64_t frame_size;
    PIXEL_FORMAT_E PixelFormat;
    uint8_t *frame_data;
    int32_t dataFd;        /* 图像数据DMA buffer fd */
    VIDEO_FRAME_INFO_S rkViFrame;
} frameInfo_vi_t;

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

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
