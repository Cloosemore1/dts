IDIR = ./include
_DEPS = dtsplayback.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
CFLAGS = -I$(IDIR)

dts: src/dtsplayback.c $(DEPS)
	gcc -o dtsplayback src/dtsplayback.c -laptx100 -lasound $(CFLAGS)

clean:
	rm -f dtsplayback