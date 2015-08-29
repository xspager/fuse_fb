#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>

int main() {
    int fd = open("/dev/cusetest", O_RDWR);

    const char* msg = "Fooooo";
    write(fd, msg, strlen(msg));

    int v = 63;
    ioctl(fd, 23, &v);
    fprintf(stderr, "value is now: %d\n", v);
    ioctl(fd, 42, &v);
    fprintf(stderr, "value is now: %d\n", v);
    close(fd);
    return 0;
}

