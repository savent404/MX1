#include "AF.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("This is a test, try test {pwd}/xxx.wav\n");
        return 0;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Open file error");
        return -1;
    }
    struct _AF_PCM st;
    int cnt = read(fd, &st, sizeof(st));
    if (cnt != sizeof(st)) {
        printf("Can't read enugh data\n");
        close(fd);
        return -2;
    }

    /* You can debug view in there */
    close (fd);
    return 0;
}