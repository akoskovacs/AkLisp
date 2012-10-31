CC= gcc
CFLAGS= -shared -fPIC -I "../../src" -ldl -lncurses
OBJECTS= ../curses.alm

all: $(OBJECTS)

../%.alm: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(OBJS)
