#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "function_config.h"

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
