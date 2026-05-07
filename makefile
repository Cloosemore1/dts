IDIR = ./include
_DEPS = dts.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
CFLAGS = -I$(IDIR)

dts: src/dts.c $(DEPS)
	gcc -o dts src/dts.c -laptx100 -lasound $(CFLAGS)

clean:
	rm -f dts