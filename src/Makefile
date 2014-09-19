# This is an alternative makefile, if you don't want to use CMake.
# It will only generate the executable and will not check the header files...

TARGET := ../aklisp
AKL_MODULE_SEARCH_PATH := $(PREFIX)/AkLisp/modules/

DEFINES := -include "../conf.h" -DVER_MAJOR=$(VER_MAJOR) \
	       -DVER_MINOR=$(VER_MINOR) -DVER_ADDITIONAL=$(VER_ADDITIONAL)

include Makefile.objs

CONFIG_OS_UNIX := y

obj-app = $(obj-app-y:%.o=$(OBJ_DIR)/%.o)
obj-lib = $(obj-lib-y:%.o=$(OBJ_DIR)/%.o)

all: aklisp 

aklisp: $(AKL_LIBRARY) $(obj-app-y)
	@echo "LINK aklisp"
	$(Q)$(CC) $(obj-app) $(LDFLAGS) $(AKL_LIBRARY) -o ../$@
	
libaklisp_static.a: $(obj-lib-y)
	$(Q)$(AR) $(ARFLAGS) $@ $(obj-lib)

$(AKLISP_SHARED): $(obj-lib-y)
	@echo "LINK $@"
	$(Q)$(CC) $(LDFLAGS) $@ $(obj-lib-y) 

%.o: %.c $(OBJ_DIR)
	@echo "CC $<"
	$(Q)$(CC) $(CFLAGS) $(DEFINES) $< -c -o $(OBJ_DIR)/$@

$(OBJ_DIR):
	$(MKDIR) $(OBJ_DIR)

gen-file:
	cat akl_tree.h > $(CTARGET)
	cat aklisp.h >> $(CTARGET)
	tail lexer.c parser.c gc.c list.c types.c lib.c lib_file.c aklisp.c -q -n +23 >> $(CTARGET)

PHONY: clean clean-cmake
clean:
	$(RM) $(OBJ_DIR) $(AKLISP_SHARED)
