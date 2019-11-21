
#define SM_FL_ALLOC 1
#define SM_FL_ACTIVE 2
#define SM_FL_SOURCED 4

struct _vp_stream_sys_s {
    uint32_t flags;
    struct {
        uint16_t feeds;
        uint16_t formats;
    } count;
    VP_FEED *feeds;
    char name[32];
};
typedef struct _vp_stream_sys_s VP_STREAM;


