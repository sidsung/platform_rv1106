#ifndef __INIT_PARAM_H__
#define __INIT_PARAM_H__

void init_video_param_list(void);

#if CONFIG_ENABLE_ROCKCHIP_YOLOV5
yolov5_init_param_t *get_yolov5_param(void);
#elif CONFIG_ENABLE_ROCKCHIP_IVA
video_iva_param_t *get_iva_param(void);
#endif

#endif
