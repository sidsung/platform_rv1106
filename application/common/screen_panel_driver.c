#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include "rv1106_common.h"

#include "screen_panel_driver.h"


int screen_panel_init(screen_panel_param_t *sp_param)
{
    int ret = -1;
    ret = rgb_lcd_init(sp_param->fb_dev_name, &sp_param->fb_dev); assert(ret == 0);
    ret = rgb_lcd_clear_screen(&sp_param->fb_dev); assert(ret == 0);

    return ret;
}

int screen_panel_deinit(screen_panel_param_t *sp_param)
{
    rgb_lcd_clear_screen(&sp_param->fb_dev);
    rgb_lcd_deinit(&sp_param->fb_dev);
    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
