#pragma once
#include <fuse.h>
#include <errno.h>
#include <dirent.h>
#include "../debug_defs.h"
#include "../vprocfs.h"

typedef struct {
    uint32_t flags;
    union {
        blockcache_record *cache;
        int fd;
        char *stat;
    } rec;
} fileinfo_t;

extern DIR *mount_dir;
extern cachedfs_info *cachedfs;
extern int errno;

