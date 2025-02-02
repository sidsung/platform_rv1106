#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef _SCREEN_PANEL_DRIVER_H_
#define _SCREEN_PANEL_DRIVER_H_

#include <linux/fb.h>
#include <sys/mman.h>

#include "rgb_lcd.h"

typedef struct {
    char fb_dev_name[32];
    screen_info_t fb_dev;
} screen_panel_param_t;

int screen_panel_init(screen_panel_param_t *sp_param);
int screen_panel_deinit(screen_panel_param_t *sp_param);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
