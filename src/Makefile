# This is an alternative makefile, if you don't want to use CMake.
# It will only generate the executable and will not check the header files...
PREFIX := /usr/local
VER_MAJOR := 0
VER_MINOR := 2
VER_ADDITIONAL := "prealpha"
AKL_MODULE_SEARCH_PATH := $(PREFIX)/AkLisp/modules/

CC = cc
LD = ld
RM = rm -rf
CP = cp
ALTCONF = config.h.alt
CONF = conf.h
CFLAGS = -ggdb # Default is debug build
CD = cd
MAKE = make
OBJECTS := obj-y
TARGET := ../aklisp
CTARGET := akl.c
CFLAGS = # -rdynamic -ldl -lreadline
SH_LDFLAGS := -fPIC -shared -Wl -soname

DEFINES := -include "../conf.h" -DVER_MAJOR=$(VER_MAJOR) \
	       -DVER_MINOR=$(VER_MINOR) -DVER_ADDITIONAL=$(VER_ADDITIONAL)

include Makefile.objs

CONFIG_UNIX := y

all: aklisp
	@echo "LINK aklisp"
	$(CC) $(OBJECTS) $(CFLAGS) -o $(TARGET)

%.o: %.c
	@echo "CC $<"
	$(CC) $(CFLAGS) $(DEFINES) $< -c 

libaklisp.so: libaklisp.so.$(VER_MAJOR).$(VER_MINOR)

libaklisp.so$(VER_MAJOR).$(VER_MINOR): $(obj-lib-y)
	@echo "LINK $@"
	$(CC) $(SH_LDFLAGS) $@ $(obj-lib-y) 

gen-file:
	cat akl_tree.h > $(CTARGET)
	cat aklisp.h >> $(CTARGET)
	tail lexer.c parser.c gc.c list.c types.c lib.c lib_file.c aklisp.c -q -n +23 >> $(CTARGET)

PHONY: clean clean-cmake
clean:
	$(RM) $(TARGET) $(OBJECTS)
