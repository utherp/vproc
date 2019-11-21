#include <stdlib.h>
#include <errno.h>

void *malign_alloc (size_t align, size_t size) {
    void *mem;
    int ret = posix_memalign(&mem, align, size);
    if (ret) {
        errno = ret;
        return NULL;
    }
    return mem;
}


