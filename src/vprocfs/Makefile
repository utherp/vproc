include makefile.common
TARGET_BASE	= /usr/local
BIN			= vprocfs
BUILD_DIR	= build
MKDIR		= /bin/mkdir
INSTALL		= /usr/bin/install
ECHO		= /bin/echo

SRCS	= main.c vprocfs.c
OBJS	= $(SRCS:.c=.o)

all		: $(BUILD_DIR)/$(BIN)
all		: CFLAGS+=$(DEBUG_FLAGS) -D_SHOW_WARNINGS_

debug	:	BIN=vprocfs_debug
debug	:	ACTNAME=debug
debug	:	CFLAGS+=$(DEBUG_FLAGS) -D_SHOW_WARNINGS_
debug	: 	CHKBLD $(OBJS) HANDLERS $(BUILD_DIR)/$(BIN)

$(BUILD_DIR)/$(BIN): CHKBLD $(OBJS) HANDLERS
	$(MKDIR) -p $(BUILD_DIR)
	echo "$(ACTNAME)" > $(BTYPE_FILE)
	$(CC) $(CFLAGS) $(OBJS) handlers/*.o $(DEPENDS) -o $(BUILD_DIR)/$(BIN)

install	:	BIN=vprocfs
install	:	CHKBLD all
	$(INSTALL)  -m 755 -st $(TARGET_BASE)/bin $(BUILD_DIR)/$(BIN)

HANDLERS:
	$(MAKE) -C handlers $(ACTNAME)

clean: 
	$(RM) $(BUILD_DIR)/$(BIN)
	$(RM) *.o
	$(RM) $(BTYPE_FILE)
	$(MAKE) -C handlers clean

remove	:	deinstall

deinstall:
	if [ -x $(BUILD_DIR)/$(BIN) ]; then $(RM) $(BUILD_DIR)/$(BIN); else $(ECHO) "Not installed!"; fi;

