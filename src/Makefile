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
OBJECTS := main.o aklisp.o lib.o lexer.o parser.o os_unix.o gc.o list.o
DEFINES := -DVER_MAJOR=0 -DVER_MINOR=1 -DVER_ADDITIONAL=\"alpha\" -DUSE_COLORS -DHAVE_READLINE -include "../conf.h"
TARGET := ../aklisp

all: $(OBJECTS)
	@echo "LINK aklisp"
	@$(CC) $(OBJECTS) -ldl -lreadline -o $(TARGET)

%.o: %.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(DEFINES) $< -c -rdynamic

.PHONY: clean clean-cmake
clean:
	$(RM) $(TARGET) $(OBJECTS)
