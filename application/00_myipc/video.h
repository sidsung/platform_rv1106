#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */
#ifndef _VIDEO_H
#define _VIDEO_H

#include "rv1106_common.h"
typedef enum {
    GET_VENC_FRAME,
    GET_VENC2_FRAME,
} get_frame_type_t;

int video_init(void);
int video_deinit(void);
int video_GetFrame(get_frame_type_t type, frameInfo_vi_t *fvi_info, void *arg);

#if CONFIG_ENABLE_SCREEN_PANEL
screen_panel_param_t *get_screen_panel_param(void);
#endif

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
