#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

size_t read_bytes (int fd, char *buf, size_t bytes) {
    size_t r, tot = bytes;
    while (bytes) {
        r = read(fd, buf, bytes);
        if (r == -1) {
            return -1;
        }
        bytes -= r;
        buf += r;
    }
    return tot;
}

size_t write_bytes (int fd, char *buf, size_t bytes) {
    size_t w, tot = bytes;
    while (bytes) {
        w = write(fd, buf, bytes);
        if (w == -1) {
            return -1;
        }
        bytes -= w;
        buf += w;
    }
    return tot;
}

