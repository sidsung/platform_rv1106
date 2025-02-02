#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <assert.h>

#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/prctl.h>

#include <errno.h>
#include <pthread.h>
#include <sys/poll.h>

#include "video.h"

static bool thread_run = true;

static void sigterm_handler(int sig) {
	fprintf(stderr, "signal %d\n", sig);
	thread_run = false;
}

int main(int argc, char *argv[])
{
    int ret = -1;

    signal(SIGINT, sigterm_handler);
    signal(SIGTERM, sigterm_handler);

    do {
        if (video_init()) {
            printf("ERROR: video_init faile, exit\n");
            break;
        }

        while (thread_run) {
            sleep(1);
        }
        printf("%s exit!\n", __func__);

        ret = 0;
    } while (0);

    printf("[%s %d] video_deinit \n", __FILE__, __LINE__);
    video_deinit();

    printf("[%s %d] main exit\n", __FILE__, __LINE__);
    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
