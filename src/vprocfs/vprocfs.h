#pragma once
#include <pthread.h>

#define PATH_BUFSIZE 1025

typedef struct {
    ino_t local_inode;
    ino_t source_inode;
    unsigned long block_size;
    unsigned long block_count;
    unsigned long blocks_cached;
    unsigned long blockcache_size;
    unsigned char cached[];
} blockcache_t;

typedef struct _bc_record {
    struct _bc_record *next;
    struct _cfs_index *cachedfs;
    unsigned int refs;
    time_t atime;
    char remove;
    char *path;
    int local_fd;
    int source_fd;
    int info_fd;
    pthread_mutex_t lock;
    unsigned char changed;
    struct timeval last_cached;
    struct stat source_stat;
    struct stat local_stat;
    blockcache_t *info;
} blockcache_record;

typedef struct _cfs_index {
    unsigned long long high_watermark;
    unsigned long long low_watermark;
    unsigned long files;
    unsigned long long bytes;
    unsigned long default_block_size;
    char filelist_changed;
    char *cache_path;
    char *source_path;
    DIR *cache_dir;
    DIR *source_dir;
    int cachelist_fd;
    pthread_mutex_t lock;
    struct _bc_record *records;
} cachedfs_info;

    // starting address of block #
#define block_addr(rec, block) (rec->info->block_size * block)
    // block # containing offset
#define block_num(rec, off) (off / rec->info->block_size)
    // bytes remaining in block from offset to end of block
#define bytes_in_block_from(rec, off) (bytes_in_block(rec, block_num(rec, off)) - (off - block_addr(rec, block_num(rec, off))))
    // total bytes in block (mainly for calculating bytes in last block)
#define bytes_in_block(rec, block) ((block < (rec->info->block_count-1))?rec->info->block_size:((block == (rec->info->block_count-1))?(rec->source_stat.st_size - block_addr(rec, block)):0))

cachedfs_info *init_cachedfs (const char *source_path, const char *cache_path);

unsigned long long read_num_from_file (const char *filename);

int read_block (blockcache_record *rec, char *buf, unsigned int block);
int read_bytes_at (blockcache_record *rec, char *buf, off_t offset, size_t size);

int write_block (blockcache_record *rec, const char *buf, unsigned int block);
int write_bytes_at (blockcache_record *rec, const char *buf, off_t offset, size_t size);

int mark_block_cached (blockcache_record *rec, unsigned int block);
int mark_block_uncached (blockcache_record *rec, unsigned int block);
int block_cached (blockcache_record *rec, unsigned int block);
void write_blockcache_info (blockcache_record *rec, int locked);

blockcache_record *open_blockcache_record (cachedfs_info *cachedfs, const char *path, int create);
void close_blockcache_record (blockcache_record *rec);

int remove_blockcache_by_path (cachedfs_info *cachedfs, const char *path);

void write_all_cachedfs_changes (cachedfs_info *cachedfs);
void cachedfs_check_usage (cachedfs_info *cachedfs);

char *cachedfs_status_report (cachedfs_info *cachedfs);
char *cachedfs_info_files (cachedfs_info *cachedfs);
char *stringify_number(long long bytes);
char *stringify_percent(double percent);
