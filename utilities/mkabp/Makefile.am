bin_PROGRAMS = mkabp
mkabp_SOURCES = src/mkabp.c src/default.h

# Generate default.h from default.asm before compilation
BUILT_SOURCES = src/default.h
CLEANFILES = src/default.bin src/default.h

src/default.h: src/default.asm
	$(NASM) -fbin $(srcdir)/src/default.asm -o src/default.bin
	xxd -i src/default.bin | sed 's/src_default_bin/default_bin/g' > src/default.h
	rm -f src/default.bin
    
clean-local:
	@rm config.status configure config.log
	@rm Makefile 
	@rm -r autom4te.cache/
	@rm aclocal.m4 
	@rm compile install-sh missing Makefile.in