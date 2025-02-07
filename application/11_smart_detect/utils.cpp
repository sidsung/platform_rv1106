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

int utils_write_file(char *file_name, uint8_t *frame_data, int frame_size)
{
    static FILE* save_file = NULL;

    if (!save_file) {
        printf("open file:%s\n", file_name);
        save_file = fopen(file_name, "a");
    }

    printf("write %d byte \n", frame_size);
    fwrite(frame_data, 1, frame_size, save_file);
    fflush(save_file);

    if (save_file) {
        printf("close file\n");
        fclose(save_file);
        save_file = NULL;
    }
    return 0;
}


int utils_read_file(char *file_name, uint8_t *data, int size)
{
    int result = 0;
    FILE* save_file = NULL;

    if (access(file_name, R_OK)) {
        return -1;
    }

    save_file = fopen(file_name, "r");

    result = fread(data, 1, size, save_file);
    printf("read %s %dByte\n", file_name, result);
    fclose(save_file);

    return result;
}
