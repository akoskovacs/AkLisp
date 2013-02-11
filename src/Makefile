# This is an alternative makefile, if you don't want to use CMake.
# It will only generate the executable and will not check the header files...
CC = gcc
LD = ld
RM = rm -rf
CP = cp
ALTCONF = config.h.alt
CONF = conf.h
CFLAGS = -ggdb # Default is debug build
CD = cd
MAKE = make
OBJECTS := main.o aklisp.o lib.o lexer.o parser.o os_unix.o gc.o list.o lib_file.o
DEFINES := -DVER_MAJOR=0 -DVER_MINOR=1 -DVER_ADDITIONAL=\"alpha\" -DUSE_COLORS -DHAVE_READLINE -include "../conf.h"
TARGET := ../aklisp
CTARGET := akl.c

all: $(OBJECTS)
	@echo "LINK aklisp"
	@$(CC) $(OBJECTS) -rdynamic -ldl -lreadline -o $(TARGET)

%.o: %.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(DEFINES) $< -c 

gen-file:
	cat akl_tree.h > $(CTARGET)
	cat aklisp.h >> $(CTARGET)
	tail lexer.c parser.c gc.c list.c types.c lib.c lib_file.c aklisp.c -q -n +23 >> $(CTARGET)

PHONY: clean clean-cmake
clean:
	$(RM) $(TARGET) $(OBJECTS)
