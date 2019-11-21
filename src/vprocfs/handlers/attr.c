#include <string.h>
#include "attr.h"

//extern unsigned long default_block_size;

/*************************************************************/

int hook_statfs (const char *path, struct statvfs *fsstat) {

    /***************************************************
     * here we want to return the statfs for the target
     * filesystem, so the reads are aligned with the 
     * block size of the filesystem which we are caching.
     * This eliminates the need for excess remote reads
     * for a local cache read.
     *      -- Stephen
     */

    int ret = fstatvfs(dirfd(cachedfs->source_dir), fsstat);
    fsstat->f_bsize = cachedfs->default_block_size;
    return 0;
/*
         struct statvfs {
           unsigned long  f_bsize;    // file system block size 
           unsigned long  f_frsize;   // fragment size 
           fsblkcnt_t     f_blocks;   // size of fs in f_frsize units 
           fsblkcnt_t     f_bfree;    // # free blocks 
           fsblkcnt_t     f_bavail;   // # free blocks for non-root 
           fsfilcnt_t     f_files;    // # inodes 
           fsfilcnt_t     f_ffree;    // # free inodes 
           fsfilcnt_t     f_favail;   // # free inodes for non-root 
           unsigned long  f_fsid;     // file system ID 
           unsigned long  f_flag;     // mount flags 
           unsigned long  f_namemax;  // maximum filename length 
         };
*/
}

/*************************************************************/

int hook_fgetattr (const char *path, struct stat *buf, struct fuse_file_info *fi) {
    _debug_flow("entered fgetattr on '%s'", path);

    fileinfo_t *inf = (fileinfo_t*)&(fi->fh);
    memcpy(buf, &(inf->rec.cache->source_stat), sizeof(struct stat));
    buf->st_blksize = inf->rec.cache->info->block_size;
    return 0;
}

/**********************************************************/

int hook_chmod(const char *path, mode_t fmode) {
    _debug_flow("entered chmod on '%s'", path);
//  fchdir(dirfd(mount_dir));
//  if (chmod(path+1, fmode)) return (errno*-1);
    return -ENOSYS;
}

/**********************************************************/

int hook_chown(const char *path, uid_t fuid, gid_t fgid) {
    _debug_flow("entered chown on '%s'", path);
//  fchdir(dirfd(mount_dir));
//  if (chown(path+1, fuid, fgid)) return (errno*-1);
    return -ENOSYS;
}

/**********************************************************/

/**********************************************************/

int hook_getattr(const char *path, struct stat *stbuf) {
    _debug_flow("entered getattr on '%s'", path);

    path++;
    /****************************************************
     * internal filehandles, settings, status, ect...
     */
    if (*(short*)path == *(short*)"##") {
        path += 2;
        /********************************************
         * info path (/##info/)
         */
        if (*(long*)path == *(long*)"info") {
            if (path[4] == '\0')
                fstat(dirfd(cachedfs->cache_dir), stbuf);
            else if (path[4] != '/') return -ENOENT;
            else if (cachedfs_info_stat(path + 5, stbuf)) return -errno;
            stbuf->st_mode &= 0777555;
            return 0;
        }
        
        /********************************************
         * /##peek and /##pass dirs
         * peek accesses underlying local fs
         * pass accesses underlying remote fs
         */
        if (*(long*)path == *(long*)"peek")
            fchdir(dirfd(cachedfs->cache_dir));
        else if (*(long*)path == *(long*)"pass")
            fchdir(dirfd(cachedfs->source_dir));
        else return -ENOENT;

        path += 4;
        if (path[0] == '\0') path = ".";
        else if (path[0] == '/') path++;
        else return -ENOENT;

        if (lstat(path, stbuf)) return -errno;
        stbuf->st_mode &= 0777555;
        return 0;
    }

    if (path[0] == '\0')
        return fstat(dirfd(cachedfs->source_dir), stbuf);
    
    fchdir(dirfd(cachedfs->source_dir));
    if (lstat(path, stbuf)) return -errno;
    return 0;
}

/**********************************************************/

int cachedfs_info_stat (const char *path, struct stat *stbuf) {
    if (strlen(path) != 5) {
        errno = ENOENT;
        return -1;
    }

    char l = path[4];
    long s = *(long*)path;

    if (
        (l == 's' && (s == *(long*)"byte" || s == *(long*)"file") || s == *(long*)"stat") ||
        (l == 'm' && (s == *(long*)"hi_w" || s == *(long*)"lo_w")) ||
        (l == 'e' && s == *(long*)"usag") ||
        (l == 'z' && s == *(long*)"blks")
    ) {
        fstat(cachedfs->cachelist_fd, stbuf);
        stbuf->st_size = 2000;
        return 0;
    }

    errno = ENOENT;
    return -1;
}


