DEBUG_FLAGS = -D_DEBUG_VERBOSE_ -D_DEBUG_POINTERS_ #-D_DEBUG_LOCKS_ -D_DEBUG_FAILURES_ 

BASEDIR =   /usr/local/careview
BUILD   =   build
CC      =   /usr/bin/gcc
CFLAGS  =   -Wall -Wno-format -I./include/ -std=c99 -pthread -mmmx -msse -msse2 -msse3 -D__SIZEOF_POINTER__=8 $(DEBUG_FLAGS) -D_SHOW_WARNINGS_ -D_SHOW_ERRORS_ -D_USE_POSIX_SHM_

SRCS	=	test.c  vproc.c vproc_mm.c  vproc_sm.c  mm_set.c  vp_feed.c  vp_fmt.c

OBJS    =   $(SRCS:.c=.o)

.c.o: $($<:.c=.h)
	$(CC) $(CFLAGS) -c $<

all     :   $(BUILD)/vproc

debug   :   DEBUG_FLAGS += -D__DEBUGGING__
debug   :   all

$(BUILD)/vproc:  $(OBJS)
	[ -d $(BUILD) ] || mkdir -p $(BUILD);
	$(CC) $(CFLAGS) $(DEPEND) $(OBJS) -o $@

clean: 
	$(RM) *.o
	$(RM) $(BUILD)/vproc

objects: $(OBJS)

