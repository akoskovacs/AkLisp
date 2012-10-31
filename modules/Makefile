# This makefile should be used from
# the project's root path
CC= gcc
RM= rm -rf
CFLAGS= -shared -ldl -I "./src"
SRCS= $(wildcard modules/*.c)
OBJS= $(patsubst %.c,%.alm,$(SRCS))
all: $(OBJS)

%.alm: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(OBJS)
