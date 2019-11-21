#include "link.h"

/**********************************************************/

int hook_readlink(const char *path, char *buf, size_t bufsize) {
    _debug_flow("entered readlink on '%s'", path);
    path++;
    fchdir(dirfd(cachedfs->source_dir));
    if (path[0] == '#' && path[1] == '#') {
        path += 2;
        unsigned long *tmp = (unsigned long*)path;
        if (*tmp == *((unsigned long*)"peek")) {
            fchdir(dirfd(cachedfs->cache_dir));
            path += 5;
        } else if (*tmp == *((unsigned long*)"pass"))
            path += 5;
        else 
            return -ENOENT;
    }

    int bytes = readlink(path, buf, bufsize);
    if (bytes == -1) return -errno;
    return 0;
}

/**********************************************************/

int hook_symlink(const char *path, const char *dest) {
    _debug_flow("entered symlink on '%s'", path);
//  fchdir(dirfd(mount_dir));
//  if (symlink(dest, path+1)) return (errno*-1);
    return -ENOSYS;

}

/**********************************************************/

int hook_link(const char *path, const char *dest) {
    _debug_flow("entered link on '%s'", path);
//  fchdir(dirfd(mount_dir));
//  if (link(path+1, dest)) return (errno*-1);
    return -ENOSYS;
}

/**********************************************************/

