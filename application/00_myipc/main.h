#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ENABLE_RTSP_PUSH       1
#define ENABLE_SCREEN_PANEL    1
#define ENABLE_ROCKCHIP_IVA    1

typedef struct {
    int cpu_usage;
    int cpu_temp;
    int npu_usage;
} system_status_t;

system_status_t *get_system_status(void);

#ifdef __cplusplus
}
#endif

#endif
