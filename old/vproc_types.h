#include <stdint.h>

#define vproc_filter struct _vproc_filter 
#define vproc_signal struct _vproc_signal
#define vproc_buffer struct _vproc_buffer
#define vproc_buffer_signals struct _vproc_buffer_signals
#define vproc_frame struct _vproc_frame 

typedef union _vproc_data_types {
    uint8_t  *uint8;
    int8_t   *int8;
    uint16_t *uint16;
    int16_t  *int16;
    uint32_t *uint32;
    int32_t  *int32;
    uint64_t *uint64;
    int64_t  *int64;
} vproc_data_types;

vproc_buffer_signals {
    vproc_signal *write;
    vproc_signal *read;
    vproc_signal *close;
    vproc_signal *open;
    vproc_signal *lock;
    vproc_signal *unlock;
};

#define VPROC_SIGCALL(name) int32_t (*name) (uint32_t id, vproc_buffer *buf, vproc_signal *next);




vproc_signal {
    uint32_t id;
    vproc_signal *next;
    VPROC_SIGCALL(call)
};


/*********** vproc frame format *************/
typedef struct _vproc_pixel_format_s {
    const char name[8];             /* up to 8 char string representing format name, no standard yet but consitering starting with fourcc */
    uint8_t    bits_per_pixel;      /* bits per pixel */
    uint8_t    channels;            /* number of channels, max 4 */
    uint8_t    bits_per_channel[4]; /* bits in each channel */
    uint8_t    flags;               /* VP_FMT_INTERLACED, VP_FMT_PLANAR, ... probably more to come */
} vproc_pixel_format;

/*********** vproc frame pool ******************/
typedef struct _vproc_frame_pool_s {
    
}




vproc_buffer {
    vproc_buffer *prev;
    vproc_buffer *next;
    uint32_t refs;
    uint32_t page_size;
    uint32_t size;
    uint32_t flags; /* aligned, pageable, shared, ... */
    vproc_buffer_signals signal;
    union _data_types as;
};



vproc_frame {
    enum _fourcc_type fourcc;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t flags;     /* memaligned, writable, interleaving?... */
    uint32_t channels;  /* max 4 */
    uint32_t channel_length[4];
    vproc_buffer *buffer;
} vproc_frame;

vproc_filter {
    uint32_t id;    /* id given upon loading */
    vproc_frame *source;
    vproc_frame *destination;
}

typedef enum _vproc_plugin_type {
    unknown,
    filter,
    module,
    client
} vproc_plugin_type;

#define vproc_plugin_info struct _vproc_plugin_info
    
vproc_support {
    vproc_format *format;
    uint32_t flags;      /* decode, encode, ... */
    vproc_plugin *

vproc_source_format {
    vproc_format *next;
    uint32_t id;
    uint32_t flags;      /* raw, ? ... */
    char name[16];
}

vproc_filter {
    vproc_plugin *plugin;
}

vproc_plugin_info {
    uint32_t id;
    vproc_plugin_type type;
    void *handle;
}


