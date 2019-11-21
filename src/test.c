#define _BSD_SOURCE
#define _VPROC_MAIN_
#define __NEED_VPROC_MM_SYS__

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#include "vproc.h"


int running = 1;
int count = 0;
pthread_mutex_t runblock = PTHREAD_MUTEX_INITIALIZER;

void *reader_exec (void *stream_ptr) {
    VP_STREAM *stream = (VP_STREAM*)stream_ptr;
    VP_FEED *feed = open_feed(stream, "myfeed", NULL);
    printf("READER: opened feed (running: %d)\n", running);

    char *buffer;
    int ret;
    pthread_mutex_lock(&runblock);

    do {
        if (pthread_mutex_trylock(&runblock) != EBUSY) break;
//        printf("READER: Aquiring buffer....\n"); fflush(stdout);
        ret = aquire_buffer(feed, (void**)&buffer);
        if (ret == -1) {
//            printf("READER: Failed aquiring buffer, timed out!\n"); fflush(stdout);
            break;
        }
//        printf("READER: aquired buffer %u  (0x%X)\n", count++, buffer); fflush(stdout);
        count++;
        usleep(1000);
    } while(1);

    printf("READER: Read %u buffers\n", count); fflush(stdout);

    return (void*)&count;
}


int main (int argc, char **argv) {
    
    vproc_init();

    FILE *input = fopen("test_hm12-720x288.yuv", "r");
    struct timeval start, end;
    int cycles=0, width = 0, height = 0;
    char bpp = 0;
    if (argc < 5 || !(width = atoi(argv[1])) || !(height = atoi(argv[2])) || !(bpp = (char) ( 255 & atoi(argv[3]))) || !(cycles = atoi(argv[4]))) {
        printf("usage: %s width height bpp cycles\n", argv[0]);
        return 1;
    }

    if (!input) {
        fprintf(stderr, "failed to open input: %s\n", strerror(errno));
        return 1;
    }

    display_mm_summary(stdout);

    VP_STREAM *stream = open_stream("test stream 1", O_CREAT);

    if (!stream) {
        printf("Error opening stream: %s\n", strerror(errno));
        return 2;
    }

    display_mm_summary(stdout);

    VP_FMT *format = create_format("TestFmt1", width, height, 3, bpp);
//    printf("Created format:\n   Width: %u\n  Height: %u\n     Bpp: %u\n    Size: %u\n", format->width, format->height, format->bpp, format->size);

    display_mm_summary(stdout);

    VP_FEED *feed = open_feed(stream, "myfeed", format);
    if (!feed) {
        printf("Error opening feed: %s\n", strerror(errno));
        return 3;
    }

    display_mm_summary(stdout);

    char *buffer = NULL;
    int ret;
    
//    int wloops = 32*1024/sizeof(int) - 1;
//    int sval=0;


    pthread_mutex_lock(&runblock);

    running = 1;
    pthread_t reader_thread;
    pthread_create(&reader_thread, NULL, reader_exec, stream);

    display_mm_summary(stdout);

    int first = 2;
    int bcount = 0;
    gettimeofday(&start, NULL);
    
    do {
        ret = aquire_buffer(feed, (void**)&buffer);
        if (first) {
            first--;
            if (!first)
                pthread_mutex_unlock(&runblock);
        }

//        int * restrict ibuf = (int*)buffer;
//        int val = sval++;
//        int i = 0;

        size_t rr = fread(buffer, format->size, 1, input);
        if (!rr) {
            printf("Failed to read input...\n");
            break;
        }
//        printf("aquired buffer: 0x%X\n", buffer);
//        printf("--> previous data: '%s'\n", buffer);

//        for (i=wloops; i; i--, ibuf[i] = val++);

        release_buffer(feed);

//        fseek(input, 0L, SEEK_SET);
        usleep(7);
//        pthread_yield();
//        printf("-->      new data: '%s'\n", buffer);

        bcount++;
    } while (cycles--);

    display_mm_summary(stdout);

    pthread_mutex_unlock(&runblock);
//    aquire_buffer(feed, (void**)&buffer);
    close_feed(feed);

    gettimeofday(&end, NULL);

    if (start.tv_usec > end.tv_usec) {
        end.tv_usec+= 1000000;
        end.tv_sec--;
    }
    end.tv_usec -= start.tv_usec;
    end.tv_sec -= start.tv_sec;

    printf("Time: %u.%06u\n", end.tv_sec, end.tv_usec);

    void *tret = NULL;
    pthread_join(reader_thread, &tret);
    int *val = tret;

    printf("wrote %u buffers\n", bcount);
    printf("Reader thread returned %d\n", *val);

    display_mm_usage(stdout);

    return 0;
}

