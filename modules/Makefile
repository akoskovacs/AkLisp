include modules.conf
include Makefile.mods

all: $(akl-mod-y)
	@$(foreach M, $^, $(MAKE) -C $M)

clean:
	$(foreach M, $(akl-mod-y), $(MAKE) -C $M clean)
