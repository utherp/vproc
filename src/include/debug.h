#ifndef __DEBUG_H__
#define __DEBUG_H__
#include <stdio.h>

#define _show_error(sec,fmt,...) do { \
    fprintf(stderr, "(0x%X) ERROR [" __FILE__ ":%u] (" sec "):  " fmt "\n", pthread_self(), __LINE__, __VA_ARGS__); \
    fflush(stderr); } while (0)

#ifdef __DEBUGGING__
    #define _debugger(sec,fmt,...) do { \
        fprintf(stderr, "(0x%X) DEBUG [" __FILE__ ":%u] (" sec "): " fmt "\n", pthread_self(), __LINE__, __VA_ARGS__); \
        fflush(stderr); } while (0)
#else
    #define _debugger(sec,fmt,...)
#endif

#endif
