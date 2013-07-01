# This makefile should be used from
# the project's root path
CC= gcc
RM= rm -rf
CFLAGS= tester.o -L .. -laklisp_shared -I tester -I ../src -ldl -ggdb
SRCS= $(wildcard *.c)
OBJS= $(patsubst %.c,%.test,$(SRCS))
LIBAKLISP=libaklisp_shared.so
export LD_LIBRARY_PATH "..:$LD_LIBRARY_PATH"

all: tester.o check-build  $(OBJS)
	@./run_tests.sh

tester.o: tester/tester.c
	@echo "CC $<"
	@$(CC) -I ../src tester/tester.c -c -DUSE_TPUT

%.test: %.c
	@echo "CC $<"
	@$(CC) $< $(CFLAGS) -o $@

check-build:
	@echo "Checking build: $(LIBAKLISP)"
	@./tester/build_checker.sh $(LIBAKLISP)

clean:
	$(RM) tester.o $(OBJS) 
