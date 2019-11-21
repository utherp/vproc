#include "file.h"
#include <string.h>
#include <stdlib.h>

/**********************************************************/

int hook_mknod(const char *path, mode_t fmode, dev_t fdev) {
    _debug_flow("entered mknod on '%s'", path);
    return -ENOSYS;
}

/**********************************************************/

int hook_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    return -ENOSYS;
}

/**********************************************************/

int hook_unlink(const char *path) {
    _debug_flow("entered unlink on '%s'", path);
    return remove_blockcache_by_path(cachedfs, path);
}

/**********************************************************/

int hook_rename(const char *path, const char *dest) {
    _debug_flow("entered rename on '%s'", path);
    return -ENOSYS;
}


/**********************************************************/

int hook_ftruncate (const char *path, off_t offset, struct fuse_file_info *fi) {
    _debug_flow("entered ftruncate on '%s'", path);
    return -ENOSYS;
}

/**********************************************************/

int hook_truncate(const char *path, off_t offset) {
    _debug_flow("entered truncate on '%s'", path);
    return -ENOSYS;
}

/**********************************************************/

int hook_lock (const char *path, struct fuse_file_info *fi, int cmd, struct flock *fl) {
    _debug_flow("entered lock on '%s'", path);
    /*
     * The cmd argument will be either F_GETLK, F_SETLK or F_SETLKW.
     *
     * For the meaning of fields in 'struct flock' see the man page
     * for fcntl(2).  The l_whence field will always be set to
     * SEEK_SET.
     *
     * For checking lock ownership, the 'fuse_file_info->owner'
     * argument must be used.
     *
     * For F_GETLK operation, the library will first check currently
     * held locks, and if a conflicting lock is found it will return
     * information without calling this method.  This ensures, that
     * for local locks the l_pid field is correctly filled in.  The
     * results may not be accurate in case of race conditions and in
     * the presence of hard links, but it's unlikly that an
     * application would rely on accurate GETLK results in these
     * cases.  If a conflicting lock is not found, this method will be
     * called, and the filesystem may fill out l_pid by a meaningful
     * value, or it may leave this field zero.
     *
     * For F_SETLK and F_SETLKW the l_pid field will be set to the pid
     * of the process performing the locking operation.
     *
     * Note: if this method is not implemented, the kernel will still
     * allow file locking to work locally.  Hence it is only
     * interesting for network filesystems and similar.
     */
}

/*************************************************************/

static char *cachedfs_info_string (const char *path) {
    if (strlen(path) != 5) {
        errno = ENOENT;
        return NULL;
    }

    char l = path[4];
    long s = *(long*)path;

    switch (l) {
        case ('s'):
            if (s == *(long*)"byte") return stringify_number(cachedfs->bytes);
            if (s == *(long*)"file") return cachedfs_info_files(cachedfs);
            if (s == *(long*)"stat") return cachedfs_status_report(cachedfs);
            break;
        case ('m'):
            if (s == *(long*)"hi_w") return stringify_number((long long)cachedfs->high_watermark);
            if (s == *(long*)"lo_w") return stringify_number((long long)cachedfs->low_watermark);
            break;
        case ('e'):
            if (s == *(long*)"usag") return stringify_percent((double)cachedfs->bytes / (double)cachedfs->high_watermark);
            break;
        case ('z'):
            if (s == *(long*)"blks") return stringify_number((long long)cachedfs->default_block_size);
            break;
    }

    errno = ENOENT;
    return NULL;
}


int hook_open(const char *path, struct fuse_file_info *fi) {
    int ret;
    _debug_flow("entered open on '%s'", path);

    fileinfo_t *inf = (fileinfo_t*)&(fi->fh);
    path++;

    fi->direct_io = 1;

    /*******************************************8
     * internal special files
     */
    if (*(short*)path == *(short*)"##") { //path[1] == '#' && path[2] == '#') {
        path += 2;
        // all special files at this level are 4 chars, and all are directories
        if ((strlen(path) < 4) || path[4] != '/') return -ENOENT;

        /********************************************
         * cachedfs info and status directory
         */
        if (*(long*)path == *(long*)"info") {
            inf->flags = 1;
            path += 5;
            inf->rec.stat = cachedfs_info_string(path);
            if (!inf->rec.stat) return -errno;
            return 0;
        }

        /*******************************************
         * pass and peek dirs (read only)
         * peek accesses underlying local fs 
         * pass accesses underlying remote fs
         */
        if ((*(long*)path == *(long*)"pass") || *(long*)path == *(long*)"peek") {
            inf->flags = 2;
            if (path[1] == 'a') fchdir(dirfd(cachedfs->source_dir));
            else fchdir(dirfd(cachedfs->cache_dir));
            path += 5;
            inf->rec.fd = open(path, O_RDONLY);
            if (inf->rec.fd == -1) return -errno;
            return 0;
        }

        // unknown special file
        return -ENOENT;
    }
        
    inf->rec.cache = open_blockcache_record(cachedfs, path, 1);

    fi->keep_cache = 1;

    _debug_flow("--blockcache handle is %x", inf->rec.cache);

    return 0;
}

/*************************************************************/

int hook_release (const char *path, struct fuse_file_info *fi) {
    _debug_flow("entered release on '%s'", path);
    fileinfo_t *inf = (fileinfo_t*)&(fi->fh);

    if (inf->flags == 1)
        free(inf->rec.stat);
    else if (inf->flags == 2)
        close(inf->rec.fd);
    else
        close_blockcache_record(inf->rec.cache);
    fi->fh = 0;
    return 0;
}

/*************************************************************/

int hook_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {

    fileinfo_t *inf = (fileinfo_t*)&(fi->fh);

    if (inf->flags == 1) {
        int len = strlen(inf->rec.stat);
        if (offset >= len) return 0;
        if (offset + size >= len) size = len - offset;
        memcpy(buf, inf->rec.stat + offset, size);
        return size;
    } else if (inf->flags == 2) {
        lseek(inf->rec.fd, offset, SEEK_SET);
        return read(inf->rec.fd, buf, size);
    }

    _debug_flow("entered read on '%s' (%x).  size:%d, off:%lld", path, inf->rec.cache, size, offset);

    int ret = read_bytes_at(inf->rec.cache, buf, offset, size);

    _debug_flow("--returning with %d bytes read", ret);
    return ret;
}

/*************************************************************/

int hook_write(const char *path, const char *buf, size_t size, off_t offset, 
                        struct fuse_file_info *fi) {

    fileinfo_t *inf = (fileinfo_t*)&(fi->fh);
    
    if (inf->flags)
        return -EACCES;

    _debug_flow("entered write on '%s' (%x). size:%d, off:%lld", path, inf->rec.cache, size, offset);

    return write_bytes_at(inf->rec.cache, buf, offset, size);
}

/**********************************************************/


