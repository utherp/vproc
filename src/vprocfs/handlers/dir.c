#include "dir.h"
#include <string.h>

/*************************************************************/

int hook_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi) {
    _debug_flow("entered readdir on '%s'", path);
    
    int ret;
    path++;

    if (*(short*)path == *(short*)"##") {
        path += 2;
        if ((strlen(path) < 4) || (path[4] != '/' && path[4] != '\0')) return -ENOENT;

        if (*(long*)path == *(long*)"info") {
            if (path[4] != '\0') return -ENOENT;
            filler(buf, ".", NULL, 0);
            filler(buf, "..", NULL, 0);
            filler(buf, "bytes", NULL, 0);
            filler(buf, "usage", NULL, 0);
            filler(buf, "files", NULL, 0);
            filler(buf, "hi_wm", NULL, 0);
            filler(buf, "lo_wm", NULL, 0);
            filler(buf, "blksz", NULL, 0);
            filler(buf, "stats", NULL, 0);
            return 0;
        }


        if (*(long*)path == *(long*)"peek") fchdir(dirfd(cachedfs->cache_dir));
        else if (*(long*)path == *(long*)"pass") fchdir(dirfd(cachedfs->source_dir));
        else return -ENOENT;

        path += 4;
        if (path[0] == '/' && chdir(path+1)) return -errno;

    } else {
        ret = fchdir(dirfd(cachedfs->source_dir));
        if ((path[0] != '\0') && (ret = chdir(path)))
            return -errno;
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    DIR *d = opendir(".");
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        _debug_flow("adding '%s' to readdir response", ent->d_name);
        filler(buf, ent->d_name, NULL, 0);
    }
    
    closedir(d);
    return 0;
}

/*************************************************************/

int hook_mkdir(const char *path, mode_t fmode) {
//    _debug_flow("entered mkdir on '%s'", path);
//    fchdir(dirfd(mount_dir));
//    if (mkdir(path+1, fmode)) return (errno*-1);
    return -ENOSYS;
}

/**********************************************************/

int hook_rmdir(const char *path) {
    _debug_flow("entered rmdir on '%s'", path);
//    fchdir(dirfd(mount_dir));
//    if (rmdir(path+1)) return (errno*-1);
    return -ENOSYS;
}

/**********************************************************/





