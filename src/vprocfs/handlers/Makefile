include ../makefile.common

SRCS	= file.c dir.c attr.c link.c

all: $(OBJS)

debug	:	ACTNAME=debug
debug	:	CFLAGS+=$(DEBUG_FLAGS) -D_SHOW_WARNINGS_
debug	:	CHKBLD all

clean: 
	$(RM) *.o

