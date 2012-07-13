# This is an alternative makefile, if you don't want to use CMake.
# It will only generate the executable and will not check the header files...
CC = gcc
RM = rm -rf
CP = cp
ALTCONF = config.h.alt
CONF = conf.h
CFLAGS = -ggdb # Default is debug build
SOURCES := main.c aklisp.c lexer.c parser.c lib.c os_unix.c gc.c
TARGET := aklisp

all:
	$(CP) $(ALTCONF) $(CONF)	
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	$(RM) $(TARGET)
