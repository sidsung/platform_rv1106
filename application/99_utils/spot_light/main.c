#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

#include "rv1106_gpio.h"

int switch_spot(int duty)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "echo %d > /sys/class/leds/spot_light/brightness", duty % 100);
    system(buf);
    return 0;
}

int main(int argc, char *argv[])
{
    int ret = -1;
    int duty = 0;

    int fd;
    struct input_event ie;

    if (access("/sys/class/leds/spot_light/brightness", F_OK)) {
        printf("not found /sys/class/leds/spot_light/brightness\n");
        return ret;
    }

    // 打开设备文件
    fd = open("/dev/input/event0", O_RDONLY);
    if (fd == -1) {
        perror("Could not open device");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        ssize_t bytes = read(fd, &ie, sizeof(ie));
        if (bytes == sizeof(ie)) {
            // 判断事件类型是否为按键事件
            if (ie.type == EV_KEY && ie.value == 1) {
                duty = duty == 0 ? 99 : 0;
                switch_spot(duty);
            }
        } else {
            perror("Error reading from device");
            break;
        }

    }

    return ret;
}
