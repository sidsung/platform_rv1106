#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef _RV1106_IVA_H_
#define _RV1106_IVA_H_

typedef int (*iva_push_frame_callback)(void);

typedef struct {
    int enable;
    const char *models_path;
    int width;
    int height;
    RockIvaImageFormat IvaPixelFormat;

    /************************/
    RockIvaHandle handle;
    pthread_t push_thread_id;
    int push_thread_run;
} video_iva_param_t;

#define iva_red_color        0xFFFF0000    // 红色
#define iva_green_color      0xFF00FF00    // 绿色
#define iva_blue_color       0xFF0000FF    // 蓝色
#define iva_yellow_color     0xFFFFFF00    // 黄色
#define iva_cyan_color       0xFF00FFFF    // 青色
#define iva_magenta_color    0xFFFF00FF    // 品红
#define iva_orange_color     0xFFFFA500    // 橙色
#define iva_purple_color     0xFF800080    // 紫色
#define iva_white_color      0xFFFFFFFF    // 白色
#define iva_black_color      0xFF000000    // 黑色

int rv1106_iva_init(video_iva_param_t *iva);
int rv1106_iva_deinit(video_iva_param_t *iva);

int rv1106_iva_push_frame(video_iva_param_t *iva, frameInfo_vi_t *Fvi_info);
int rv1106_iva_push_frame_fd(video_iva_param_t *iva, frameInfo_vi_t *Fvi_info);

int rv1106_iva_get_result(smart_detect_result_obj_item_t *item);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
