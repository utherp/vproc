/********************************************************************
 * Synopsis:
 *
 * blockcache_record *rec = open_blockcache_record(const char *path_to_cached_file, int create);
 *
 * will return a blockcache_record structure after potentially performing
 * the following operations:
 *
 *        load the blockcache info data (rec->info)
 * OR
 *         create the blockcache info data (rec->info)
 * AND MAYBE
 *         creating the local cache file (rec->local_fd)
 *
 *     The only percevable time this call should fail is if the source file
 *     does not exist or cannot be stat'd for whatever reason.
 *
 * blockcache info data is accessable via:
 *         blockcache_t *info = rec->info;
 * 
 * file descriptors:
 *     rec->source_fd:        source file
 *     rec->local_fd:        local cached file
 *     rec->info_fd:        blockcache_t persistent info file
 *
 *
 * Useful function calls and macros:
 *
 * block address:
 *     block_addr(blockcache_t*, int block_number)
 *
 * block # of an address:
 *   block_num(blockcache_t*, off_t offset)
 *
 * # of bytes from an offset to the end of the block its contained in:
 *   bytes_in_block_from (blockcache_t*, off_t offset)
 *
 * mark a block as cached:
 *   mark_block_cached (blockcache_t*, int block_number)
 *
 * mark a block as not cached:
 *   mark_block_uncached (blockcache_t*, int block_number)
 *
 * determine if a block is cached:
 *   block_cached (blockcache_t*, int block_number)
 *
 * determine how many of the bytes requested, if any, are cached at the given offset:
 *   range_cached (blockcache_t*, off_t offset, size_t bytes_requested);
 *   ... returns int from 0 to bytes_requested
 *
 * free memory, close file descriptors and remove blockcache record from link list:
 *     free_blockcache_record (blockcache_record *)
 *
 * write blockcache info data to info data file (dirname(path) . '/.' . basename(path) . '.blockcache')
 *      write_blockcache_info (blockcache_record *);
 *
 *
 * any other functions you see here should probably not be used directly
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include "vprocfs.h"
#include "debug_defs.h"

#include "config.h"

/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/

//extern DIR *cache_dir;
//extern DIR *source_dir;

static blockcache_record *cache_records = NULL;
static pthread_mutex_t record_list_lock = PTHREAD_MUTEX_INITIALIZER;

static inline blockcache_t *make_blockcache (ino_t source_inode, ino_t local_inode, size_t filesize, size_t block_size);
static inline blockcache_record *make_blockcache_record (cachedfs_info *cachedfs, const char *path);
static blockcache_record *create_cache (cachedfs_info *cachedfs, const char *path);
static int read_cache_info (blockcache_record *listent, struct stat *stbuf);
static blockcache_record *load_cache_info (cachedfs_info *cachedfs, const char *path, struct stat *stbuf);
static inline char *cache_info_filename (const char *path, char *buf);
static void remove_from_record_list (blockcache_record *rec, int locked);
static void add_to_record_list (blockcache_record *rec);
static void ref_blockcache (blockcache_record *rec);
static void free_blockcache_record (blockcache_record *rec, int locked);

static int remove_blockcache (blockcache_record *rec, int locked);
static void sort_blockcache_records (cachedfs_info *cachedfs, int locked);
static int cache_block (blockcache_record *rec, char *buf, unsigned int block);

#define lock_cachedfs(cfs) pthread_mutex_lock(&(cfs->lock))
#define unlock_cachedfs(cfs) pthread_mutex_unlock(&(cfs->lock))
#define initlock_cachedfs(cfs) pthread_mutex_init(&(cfs->lock), NULL)
#define trylock_cachedfs(cfs) pthread_mutex_trylock(&(cfs->lock))


/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/

// marks that a block has been cached
int mark_block_cached (blockcache_record *rec, unsigned int block) {
    unsigned int cn = block>>3;
    if (cn > rec->info->block_count) return 0;
    if (!block_cached(rec, block)) {
        rec->changed = 1;
        rec->info->cached[cn] |= 1<<(block & 7);
        rec->info->blocks_cached++;
        rec->cachedfs->bytes += rec->info->block_size;
    }
    return 1;
}

// marks that a block has not been cached (block purged from cache)
int mark_block_uncached (blockcache_record *rec, unsigned int block) {
    unsigned int cn = block>>3;
    if (cn > rec->info->block_count) return 0;
    if (block_cached(rec, block)) {
        rec->changed = 1;
        rec->info->cached[cn] &= ~(1<<block & 7);
        rec->info->blocks_cached--;
        rec->cachedfs->bytes -= rec->info->block_size;
    }
    return 1;
}

// returns whether a certain block has been cached
int block_cached (blockcache_record *rec, unsigned int block) {
    unsigned int cn = block>>3;
    if (cn > rec->info->block_count) return 0;
    return (rec->info->cached[cn] & 1<<(block & 7))?1:0;
}

// returns a number up to 'size' for how many bytes are cached starting at the given offset
int range_cached (blockcache_record *rec, off_t offset, size_t size) {
    unsigned int blk = block_num(rec, offset);
    if (!block_cached(rec, blk)) return 0;
    size_t remain = bytes_in_block_from(rec, offset);
    if (remain < size) size = remain;
    return size;
}

int remove_blockcache_by_path (cachedfs_info *cachedfs, const char *path) {
    fchdir(dirfd(cachedfs->cache_dir));
    if (path[0] == '/') path++;
    blockcache_record *rec = open_blockcache_record(cachedfs, path, 0);
    if (rec == NULL) return -ENOENT;
    return remove_blockcache(rec, 0);
}

static int remove_blockcache (blockcache_record *rec, int locked) {
    char buf[PATH_BUFSIZE];
    cachedfs_info *cachedfs = rec->cachedfs;

    _debug_flow("removing blockcache with the following path: (%s)", rec->path);

    fchdir(dirfd(cachedfs->cache_dir));

    rec->remove = 1;

    unlink(rec->path);
    unlink(cache_info_filename(rec->path, buf));

    if (!locked) lock_cachedfs(cachedfs);

    cachedfs->bytes -= (rec->info->block_size * rec->info->blocks_cached);
    cachedfs->files--;
    cachedfs->filelist_changed = 1;

    free_blockcache_record(rec, 1);

    if (!locked) unlock_cachedfs(cachedfs);
    write_cached_files_list(cachedfs);

    return 0;
}

/****************************************************/

blockcache_record *open_blockcache_record (cachedfs_info *cachedfs, const char *path, int create) {
    if (path[0] == '/') path++;
    _debug_flow("opening blockcache record for '%s'", path);
    lock_cachedfs(cachedfs);
//    pthread_mutex_lock(&record_list_lock);
    _debug_flow("RECORD LIST: LOCKED", 0);

    blockcache_record *rec = cachedfs->records;
    fchdir(dirfd(cachedfs->cache_dir));

    struct stat stbuf;
    if (stat(path, &stbuf)) {
        if (create) { 
            rec = create_cache(cachedfs, path);
            if (rec) {
                cachedfs->files++;
                cachedfs->filelist_changed = 1;
                rec->next = cachedfs->records;
                cachedfs->records = rec;
                write_cached_files_list(cachedfs);
            }
        } else rec = NULL;
        unlock_cachedfs(cachedfs);
        _debug_flow("RECORD LIST: UNLOCKED", 0);
        return rec;
    }

    ino_t inode = stbuf.st_ino;
    _debug_flow("-- searching record list for inode %d", inode);
    while (rec) {
        _debug_flow("-- record list ref (%x)", rec);
        if (rec->info && (inode == rec->info->local_inode)) {
            _debug_flow("-- found previously open record...", 0);
            unlock_cachedfs(cachedfs);
            _debug_flow("RECORD LIST: UNLOCKED", 0);
            ref_blockcache(rec);
            return rec;
        }
        rec = rec->next;
    }
    unlock_cachedfs(cachedfs);
    _debug_flow("RECORD LIST: UNLOCKED", 0);

    _debug_flow("-- record not found for path '%s'", path);
    return NULL;
//    return load_cache_info(path, &stbuf);
}

static inline char *cache_info_filename (const char *path, char *buf) {
    if (path[0] == '/') path++;
    char *ptmp = strdup(path);
    char *bn = basename(ptmp);
    char *dn = dirname(ptmp);
    buf[0] = '\0';
    strcpy(buf, dn);
    strcat(buf, "/.");
    strcat(buf, bn);
    strcat(buf, ".blockcache");
    free(ptmp);
    return buf;
}

static blockcache_record *load_cache_info (cachedfs_info *cachedfs, const char *path, struct stat *stbuf) {
    // chop leading '/' from path
    if (path[0] == '/') path++;

    _debug_flow("loading blockcache info for '%s'", path);

    // change to local cache dir
    fchdir(dirfd(cachedfs->cache_dir));

    // stat local file if stat was not passed (or return NULL on failure)
    if (stbuf == NULL && stat(path, stbuf)) return NULL;

    // create blockcache link-list entry
    blockcache_record *rec = make_blockcache_record(cachedfs, path);

    // change back to local cache dir
    fchdir(dirfd(cachedfs->cache_dir));

    // open local cache file
    rec->local_fd = open(path, O_RDWR);

    if (rec->local_fd == -1) {
        _debug_flow("Error: unable to open local file: %s", strerror(errno));
        return NULL;
    }

    fstat(rec->local_fd, &(rec->local_stat));
    rec->atime = rec->local_stat.st_atime;

    // stat cache info file, read if exists, create if not
    if (read_cache_info(rec, stbuf) < 0)
        return NULL;

    return rec;
}

static int read_cache_info (blockcache_record *rec, struct stat *stbuf) {
    // get the cache info filename
    char tmp[PATH_BUFSIZE];
    cache_info_filename(rec->path, tmp);

    _debug_flow("reading cache info file (%s)...", tmp);

    fchdir(dirfd(rec->cachedfs->cache_dir));

    // stat local file if stat was not passed (or return NULL on failure)
    if (stbuf == NULL && stat(rec->path, stbuf)) {
        _debug_flow("-- error, could not stat local file: %s", strerror(errno));
        return -errno;
    }

    // attempt to stat the cache info file, if it does not exist, create it and return
    struct stat infostat;
    if (stat(tmp, &infostat)) {
        _debug_flow("-- failed to stat the cache info file... creating", 0);
        // cache info file does not exist... create it.
        rec->info = make_blockcache(rec->source_stat.st_ino, stbuf->st_ino, rec->source_stat.st_size, rec->cachedfs->default_block_size);
        write_blockcache_info(rec, 0);
        return 0;
    }

    // cache info file exists, read it...

    // open cache info file
    rec->info_fd = open(tmp, O_RDWR);

    if (rec->info_fd == -1) {
        _debug_flow("Error: unable to open cache info file: %s", strerror(errno));
        return -errno;
    }

    char *buf = calloc(1, infostat.st_size);
    int rd = 0, r = 0, rm = infostat.st_size;
    rec->info = (blockcache_t*)buf;

    lseek(rec->info_fd, 0L, SEEK_SET);
    while (rm) {
        r = read(rec->info_fd, buf, rm);
        if (!r) break;
        if (r == -1) {
            _debug_flow("-- error: failed reading cache info file: %s", strerror(errno));
            free_blockcache_record(rec, 0);
            return -errno;
        }
        rm -= r;
        buf += r;
        rd += r;
    }

    _debug_flow("-- read %d bytes from info file", rd);
    
    return 0;
}

static int make_containing_cache_path (const char *path) {
    if (path[0] == '/') path++;
    char *tmp = strdup(path);
    char *cur;

    for (cur = tmp; *cur != '\0'; ) {
        for (; *cur != '/' && *cur != '\0'; cur++);
        if (*cur == '\0') break;
        *cur = '\0';
        mkdir(tmp, 0755);
        *cur = '/';
        cur++;
    }

    free(tmp);
    return 0;

}

static blockcache_record *create_cache (cachedfs_info *cachedfs, const char *path) {
    // chop initial '/' from path
    if (path[0] == '/') path++;

    _debug_flow("creating local cache for '%s'", path);

    // allocate link list entry
    blockcache_record *rec = make_blockcache_record(cachedfs, path);

    // change to cache dir
    fchdir(dirfd(cachedfs->cache_dir));

    make_containing_cache_path(path);

    // open / create local cache file
    _debug_flow("creating local cache file...", 0);
    rec->local_fd = open(path, O_RDWR | O_CREAT, 0644);
    if (rec->local_fd == -1) {
        _debug_flow("Unable to create local cache file: %s", strerror(errno));
    }

    // stat local file for inode
//    struct stat sttmp;
    fstat(rec->local_fd, &(rec->local_stat));
    rec->atime = rec->local_stat.st_atime;
//    stat(path, &sttmp);

    // allocate blockcache info (source inode, local inode, source file size, cache block size) 
    rec->info = make_blockcache(rec->source_stat.st_ino, rec->local_stat.st_ino, rec->source_stat.st_size, rec->cachedfs->default_block_size);
    cache_block(rec, NULL, 0);
    write_blockcache_info(rec, 1);

    // seek to last byte and write a null to create the sparse file
    lseek(rec->local_fd, block_addr(rec, rec->info->block_count) - 1, SEEK_SET);
    if (write(rec->local_fd, "", 1) == -1) {
        _debug_flow("Error: Unable to write last byte for local sparse file: %s", strerror(errno));
    }

    // return blockcache list entry
    return rec;
}

static void add_to_record_list (blockcache_record *rec) {
    // add the blockcache info to the cache_records list
    lock_cachedfs(rec->cachedfs);
    _debug_flow("RECORD LIST: LOCKED", 0);
    rec->next = rec->cachedfs->records;
    rec->cachedfs->records = rec;
    unlock_cachedfs(rec->cachedfs);
    _debug_flow("RECORD LIST: UNLOCKED", 0);
    return;
}

static void remove_from_record_list (blockcache_record *rec, int locked) {
    if (!locked) {
        lock_cachedfs(rec->cachedfs);
        _debug_flow("RECORD LIST: LOCKED", 0);
    }
    if (rec->cachedfs->records  == rec) {
        rec->cachedfs->records = rec->next;
    } else {
        _debug_flow("-- searching for record to remove in list", 0);
        _debug_flow("## cachedfs ref is %x", rec->cachedfs);
        blockcache_record *tmp = rec->cachedfs->records;
        while (tmp) {
            if (tmp->next == rec) {
                _debug_flow("-- found it, setting next (%x) to this next (%x)", tmp->next, rec->next);
                tmp->next = rec->next;
                break;
            }
            tmp = tmp->next;
        }
        _debug_flow("-- done searching for record in list!", 0);
    }
    if (!locked) {
        unlock_cachedfs(rec->cachedfs);
        _debug_flow("RECORD LIST: UNLOCKED", 0);
    }
    return;
}

static inline blockcache_record *make_blockcache_record (cachedfs_info *cachedfs, const char *path) {
    blockcache_record *tmp = (blockcache_record*)calloc(1, sizeof(blockcache_record));
    tmp->cachedfs = cachedfs;
    fchdir(dirfd(cachedfs->source_dir));
    if (path[0] == '/') path++;
    tmp->path = strdup(path);
    tmp->source_fd = open(path, O_RDONLY);
    fstat(tmp->source_fd, &(tmp->source_stat));
    gettimeofday(&(tmp->last_cached));
    initlock_cachedfs(tmp);
    tmp->refs = 1;
    return tmp;
}

static inline blockcache_t *make_blockcache (ino_t source_inode, ino_t local_inode, size_t filesize, size_t block_size) {
    size_t blocks = filesize / block_size;
    if (filesize % block_size) blocks++;
    size_t chars = blocks / sizeof(char);
    if (blocks % sizeof(char)) chars++;
    blockcache_t *info = calloc(1, sizeof(blockcache_t) + chars);
    info->blockcache_size = chars;
    info->block_size = block_size;
    info->block_count = blocks;
    info->local_inode = local_inode;
    info->source_inode = source_inode;
    _debug_flow("created blockcache info: filesize(%d), block_size(%d), blocks(%d)", filesize, block_size, blocks); 
    return info;
}

static void ref_blockcache (blockcache_record *rec) {
    lock_cachedfs(rec);
    rec->refs++;
    unlock_cachedfs(rec);
    return;
}

void close_blockcache_record (blockcache_record *rec) {
    _debug_flow("closing blockcache record", 0);
    lock_cachedfs(rec);
    rec->refs--;
    write_blockcache_info(rec, 1);
    unlock_cachedfs(rec);
    return;
}

static void free_blockcache_record (blockcache_record *rec, int locked) {
    _debug_flow("freeing blockcache record", 0);
    remove_from_record_list(rec, locked);
    if (rec->info_fd) close(rec->info_fd);
    if (rec->source_fd) close(rec->source_fd);
    if (rec->local_fd) close(rec->local_fd);
    if (!rec->remove)
        write_blockcache_info(rec, 1);
    free(rec->info);
    if (rec->path) free(rec->path);
    free(rec);
    _debug_flow("-- blockcache record freed", 0);
    return;
}

void write_blockcache_info (blockcache_record *rec, int locked) {
    if (!rec->changed || rec->remove) return;
    if (!locked && trylock_cachedfs(rec)) return;
    if (!rec->info_fd) {
        char tmp[PATH_BUFSIZE];
        rec->info_fd = open(cache_info_filename(rec->path, tmp), O_RDWR | O_CREAT, 0644);
    } else
        lseek(rec->info_fd, 0L, SEEK_SET);

    write(rec->info_fd, &(rec->info->local_inode), sizeof(ino_t) * 2);
    write(rec->info_fd, &(rec->info->block_size), sizeof(unsigned long) * 3);
    write(rec->info_fd, rec->info->cached, rec->info->blockcache_size);

    fdatasync(rec->info_fd);

    if (!locked) unlock_cachedfs(rec);

    return;
}

void write_all_cachedfs_changes (cachedfs_info *cachedfs) {
    _debug_flow("writing cachedfs changes to info files...", 0);
    lock_cachedfs(cachedfs);
    _debug_flow("RECORD LIST: LOCKED", 0);
    write_cached_files_list(cachedfs);

    blockcache_record *rec = cachedfs->records;
    while (rec) {
        write_blockcache_info(rec, 0);
        rec = rec->next;
    }

    unlock_cachedfs(cachedfs);
    _debug_flow("RECORD LIST: UNLOCKED", 0);

    return;
}

int read_block (blockcache_record *rec, char *buf, unsigned int block) {
    int ret = 0;
    if (block >= rec->info->block_count) return 0;
    int rem = bytes_in_block(rec, block);
    if (!block_cached(rec, block))
        return cache_block(rec, buf, block);
    
    int r = 0;
    char *tbuf = buf;
    off_t offset = block_addr(rec, block);
    lseek(rec->local_fd, offset, SEEK_SET);

    while (rem) {
        r = read(rec->local_fd, tbuf, rem);
        if (r == -1) return -errno;
        tbuf += r;
        ret += r;
        rem -= r;
    }
    rec->atime = time(NULL);
    return ret;
}

int read_bytes_at (blockcache_record *rec, char *buf, off_t offset, size_t size) {
    int block = block_num(rec, offset);
    int ret = 0;
    if (block >= rec->info->block_count) return 0;
    size_t bytes = bytes_in_block_from(rec, offset);

    _debug_flow("reading %d bytes at %lld (local_fd:%d, source_fd:%d, block %d, bytes in block: %d)", size, offset, rec->local_fd, rec->source_fd, block, bytes);

    off_t block_offset = block_addr(rec, block);
    if (!block_cached(rec, block) && (ret = cache_block(rec, NULL, block)) < 0)
        return ret;
    
    char *tbuf = buf;
    int rem = bytes, r = 0;
    if (rem > size) rem = size;
    ret = 0;
    lseek(rec->local_fd, offset, SEEK_SET);
    while (rem) {
        r = read(rec->local_fd, tbuf, rem);
        if (r == -1) return -errno;
        rem -= r;
        tbuf += r;
        ret += r;
    }
    
    _debug_flow("read %d bytes from local cache", ret);

    rec->atime = time(NULL);
    return ret;
}

static void sort_blockcache_records (cachedfs_info *cachedfs, int locked) {
    if (!cachedfs->records) return;
    if (!locked) lock_cachedfs(cachedfs);

    blockcache_record *old;

    #ifdef _DEBUG_FLOW_
        _debug_flow("Pre-sort:", 0);
        for (old = cachedfs->records; old; old = old->next) {
            _debug_flow("\t%u: %s", old->atime, old->path);
        }
    #endif

    do {
        for (old = cachedfs->records; old->next && old->next->atime <= old->atime; old = old->next);
        if (old->next) {
            blockcache_record *tmp = old->next;
            old->next = tmp->next;
            tmp->next = cachedfs->records;
            cachedfs->records = tmp;
            continue;
        }
        break;
    } while (1);

    #ifdef _DEBUG_FLOW_
        _debug_flow("Post-sort:", 0);
        for (old = cachedfs->records; old; old = old->next) {
            _debug_flow("\t%u: %s", old->atime, old->path);
        }
    #endif

    if (!locked) unlock_cachedfs(cachedfs);
    return;
}

int write_block (blockcache_record *rec, const char *buf, unsigned int block) {
    int rem = bytes_in_block(rec, block);
    if (block == (rec->info->block_count - 1))
        rem = rec->source_stat.st_size - block_addr(rec, block);
    int wr = 0;
    lseek(rec->local_fd, block_addr(rec, block), SEEK_SET);
    while (rem) {
        wr = write(rec->local_fd, buf, rem);
        if (wr == -1)
            return -errno;
        rem -= wr;
        buf += wr;
    }
    mark_block_cached(rec, block);
    return rec->info->block_size;
}

int write_bytes_at (blockcache_record *rec, const char *buf, off_t offset, size_t size) {
    int block = block_num(rec, offset);
    if (block >= rec->info->block_count) return 0;
    off_t block_offset = block_addr(rec, block);
    int ret = 0;
    const char *buf_ref = buf;
    if (block_offset != offset) {
        size -= (offset - block_offset);
        buf_ref += (offset - block_offset);
        block++;
        block_offset += rec->info->block_size;
    }

    while (size > 0) {
        if (size >= rec->info->block_size || block == (rec->info->block_count - 1)) 
            ret += write_block(rec, buf_ref, block);
        block++;
        if (block == (rec->info->block_count)) return ret;
        block_offset += rec->info->block_size;
        size -= rec->info->block_size;
        buf_ref += rec->info->block_size;
    }

    return ret;
}

static int cache_block (blockcache_record *rec, char *buf, unsigned int block) {
    if (block >= rec->info->block_count) return 0;
    _debug_flow("caching block %d", block);
    off_t offset = block_addr(rec, block);
    size_t bytes = bytes_in_block(rec, block);
    int freeit = 0;
    char *btmp;
    int ret = 0;
    if (buf == NULL) {
        freeit = 1;
        buf = malloc(bytes);
    }
    btmp = buf;

    int rem = bytes, r = 0;

    lseek(rec->source_fd, offset, SEEK_SET);

    while (rem) {
        r = read(rec->source_fd, btmp, rem);
        if (r == -1) {
            ret = -errno;
            break;
        }
        rem -= r;
        btmp += r;
        ret += r;
    }
    if (ret > 0) {
        _debug_flow("read %d bytes from source, writing block to local cache...", ret);
        ret = write_block(rec, buf, block);
    }

    if (freeit) free(buf);
    return ret;
}

/*****************************************************************************************/

cachedfs_info *init_cachedfs (const char *cache_path, const char *source_path) {
    cachedfs_info *cachedfs = calloc(1, sizeof(cachedfs_info));
    initlock_cachedfs(cachedfs);
    cachedfs->source_path = strdup(source_path);
    cachedfs->cache_path = strdup(cache_path);

    _debug_flow("Cachefs init:\n\tsource: '%s'\n\tlocal: '%s'\n", cachedfs->source_path, cachedfs->cache_path);

    cachedfs->source_dir = opendir(cachedfs->source_path);
    if (!cachedfs->source_dir) {
        _show_error("ERROR: Unable to open source dir!: %s", strerror(errno));
        exit(8);
    }

    cachedfs->cache_dir = opendir(cachedfs->cache_path);
    if (!cachedfs->cache_dir) {
        _show_error("ERROR: Unable to open cache dir!: %s", strerror(errno));
        exit(8);
    }

    cachedfs->default_block_size = (unsigned long)read_num_from_file(".cachedfs_blocksize");
    if (!cachedfs->default_block_size)
        cachedfs->default_block_size = DEFAULT_BLOCK_SIZE;

    read_watermarks(cachedfs);
    read_cached_files_list(cachedfs);

//    unsigned long low_watermark, unsigned long high_watermark
    return cachedfs;
}

unsigned long long read_num_from_file (const char *filename) {
    struct stat sttmp;
    if (stat(filename, &sttmp)) return 0;
    // if filesize is rediculous, forget it!
    if (sttmp.st_size > 255) return 0;

    char buff[256];
    buff[255] = '\0';
    char *tmp = buff;

    int fd = open(filename, O_RDONLY);
    if (fd == -1) return 0;

    int r, rm = 255;

    while (rm) {
        r = read(fd, tmp, rm);
        if (r == -1) {
            close(fd);
            return 0;
        }
        if (!r) break;
        rm -= r;
        tmp += r;
    }

    close(fd);
    return atoll(buff);
}
    
int read_watermarks (cachedfs_info *cachedfs) {
    fchdir(dirfd(cachedfs->cache_dir));
    cachedfs->high_watermark = read_num_from_file(".cachedfs_high_watermark");
    cachedfs->low_watermark = read_num_from_file(".cachedfs_low_watermark");
    if (!cachedfs->high_watermark)
        cachedfs->high_watermark = DEFAULT_HIGH_WATERMARK;
    if (!cachedfs->low_watermark)
        cachedfs->low_watermark = DEFAULT_LOW_WATERMARK;
    return 0;
}

int read_cached_files_list (cachedfs_info *cachedfs) {
    fchdir(dirfd(cachedfs->cache_dir));
    cachedfs->cachelist_fd = open(".cached_files", O_RDWR);
    if (cachedfs->cachelist_fd == -1) {
        cachedfs->cachelist_fd = open(".cached_files", O_CREAT | O_RDWR, 0644);
        if (cachedfs->cachelist_fd == -1) {
            _show_error("ERROR: Unable to open cached files info list '.cached_files': %s", strerror(errno));
            exit(10);
        }
        cachedfs->filelist_changed = 1;
        write_cached_files_list(cachedfs);
        return 0;
    }

    struct stat sttmp;
    fstat(cachedfs->cachelist_fd, &sttmp);
    int r, rm, listsize = sttmp.st_size;

    char *buffer = malloc(listsize + 1);
    buffer[listsize] = '\0';
    char *btmp = buffer;

    blockcache_record *last = NULL;

    rm = listsize;
    while (rm) {
        r = read(cachedfs->cachelist_fd, btmp, rm);
        if (r == -1) {
            _show_error("WARNING: Unable to read cached files info list: %s", strerror(errno));
            exit(11);
        }
        if (!r) {
            btmp[0] = '\0';
            break;
        }
        rm -= r;
        btmp += r;
    }

    blockcache_record *rec = NULL;

    btmp = buffer;
    for (btmp = buffer; btmp < (buffer + listsize); btmp += strlen(btmp) + 1) {
        if (stat(btmp, &sttmp)) {
            _debug_flow("WARNING: Unable to stat cache file '%s': %s", btmp, strerror(errno));
            continue;
        }
        rec = load_cache_info(cachedfs, btmp, &sttmp);
        if (!rec) {
            _debug_flow("WARNING: Unable to load cache info for file '%s'", btmp);
            continue;
        }

        if (!last) {
            rec->next = cachedfs->records;
            cachedfs->records = rec;
        } else {
            last->next = rec;
            rec->next = NULL;
        }
        last = rec;
        /*********************************************************************
         * order records by access time starting with most recently accessed
         */
        /*
        if (!cachedfs->records || cachedfs->records->atime < rec->atime) {
            rec->next = cachedfs->records;
            cachedfs->records = rec;
        } else {
            blockcache_record *tmprec = cachedfs->records;
            for (; tmprec->next && tmprec->next->atime > rec->atime; tmprec = tmprec->next);
            rec->next = tmprec->next;
            tmprec->next = rec;
        }
        */

        cachedfs->files++;
        cachedfs->bytes += (rec->info->block_size * rec->info->blocks_cached);
    }

    free(buffer);

    sort_blockcache_records(cachedfs, 1);

    cachedfs->filelist_changed = 1;
    write_cached_files_list(cachedfs);
    return cachedfs->files;
}

int write_cached_files_list (cachedfs_info *cachedfs) {
    if (!cachedfs->filelist_changed) return 0;
    fchdir(dirfd(cachedfs->cache_dir));
    lseek(cachedfs->cachelist_fd, 0L, SEEK_SET);

    _debug_flow("Writing the list of cached files into '.cached_files'...", 0);
    int w, wm;
    char *tmp;

    blockcache_record *rec;
    for (rec = cachedfs->records; rec; rec = rec->next) {
        wm = strlen(rec->path) + 1;
        tmp = rec->path;
        while (wm) {
            w = write(cachedfs->cachelist_fd, tmp, wm);
            if (w == -1) {
                _show_error("ERROR: Unable to write cached files info list!: %s", strerror(errno));
                exit(12);
            }
            tmp += w;
            wm -= w;
        }
    }

    ftruncate(cachedfs->cachelist_fd, lseek(cachedfs->cachelist_fd, 0L, SEEK_CUR));
    fdatasync(cachedfs->cachelist_fd);

    cachedfs->filelist_changed = 0;

    _debug_flow("Done writing cached files list", 0);
    return 0;
}

/****************************************************************************/

void cachedfs_check_usage (cachedfs_info *cachedfs) {
    if (cachedfs->bytes < cachedfs->high_watermark) return;
    if (!cachedfs->records) return;

    _debug_flow("NOTE: Bytes used (%llu) is %llu bytes over high watermark (%llu)!", cachedfs->bytes, cachedfs->bytes - cachedfs->high_watermark, cachedfs->high_watermark);
    _debug_flow("--> Purging %llu bytes to low watermark (%llu)", cachedfs->bytes - cachedfs->low_watermark, cachedfs->low_watermark);
        
    lock_cachedfs(cachedfs);

    sort_blockcache_records(cachedfs, 1);

    blockcache_record **rev = malloc(sizeof(void*) * cachedfs->files);
    blockcache_record *tmp = cachedfs->records;

    int i, removed = 0;
    for (i = cachedfs->files - 1; i >= 0; i--) {
        rev[i] = tmp;
        tmp = tmp->next;
    }
    
    long long start_use = cachedfs->bytes;

    i = 0;
    while ((cachedfs->bytes > cachedfs->low_watermark) && i < cachedfs->files) {
        remove_blockcache(rev[i++], 1);
        removed++;
    }

    _debug_flow("--> Removed %u cached files, freeing a total of %llu bytes", removed, start_use - cachedfs->bytes);

    unlock_cachedfs(cachedfs);

    free(rev);

    _debug_flow("--> Bytes used is now %llu", cachedfs->bytes);

    return;
}

/****************************************************************************/

char *cachedfs_status_report (cachedfs_info *cachedfs) {
    char *report = calloc(1, 1000);
    snprintf(report, 1000, 
        "Files: %u\n"
        "block size: %u\n"
        "Watermarks:\n"
        "    High: %llu\n"
        "     Low: %llu\n\n"
        "bytes used: %llu\n"
        "remain before purging: %lld\n\n",
        cachedfs->files,
        cachedfs->default_block_size, 
        cachedfs->high_watermark,
        cachedfs->low_watermark, 
        cachedfs->bytes,
        cachedfs->high_watermark - cachedfs->bytes
    );
    return report;
}

char *cachedfs_info_files (cachedfs_info *cachedfs) {
    struct stat sttmp;
    fstat(cachedfs->cachelist_fd, &sttmp);
    char *tmp = calloc(1, sttmp.st_size + 1);

    int r, rm = sttmp.st_size;
    char *cur = tmp;

    lseek(cachedfs->cachelist_fd, 0L, SEEK_SET);
    while (rm) {
        r = read(cachedfs->cachelist_fd, cur, rm);
        if (r == -1) {
            snprintf(tmp, sttmp.st_size, "An error occurred while listing the cached files: %s\n", strerror(errno));
            return tmp;
        }
        cur += r;
        rm -= r;
    }
    for (r = 0; r < sttmp.st_size; r++)
        if (tmp[r] == '\0') tmp[r] = '\n';
    
    return tmp;
}

char *stringify_number(long long bytes) {
    char *tmp = malloc(50);
    snprintf(tmp, 50, "%llu", bytes);
    return tmp;
}

char *stringify_percent(double percent) {
    char *tmp = malloc(10);
    percent *= 100;
    snprintf(tmp, 10, "%.4f%%", percent);
    return tmp;
}


