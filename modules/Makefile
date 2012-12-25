# This makefile should be used from
# the project's root path
CC= gcc
RM= rm -rf
CFLAGS= -shared -fPIC -ldl -ggdb -I "../src"
SRCS= $(wildcard *.c)
OBJS= $(patsubst %.c,%.alm,$(SRCS))
all: $(OBJS)

%.alm: %.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(OBJS)
