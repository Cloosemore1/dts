IDIR = ./include
_DEPS = dtsplayback.h dtstimecode.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
CFLAGS = -I$(IDIR)

.PHONY: all

all: dtsplayback dtstimecode

dtsplayback: src/dtsplayback.c $(DEPS)
	gcc -o dtsplayback src/dtsplayback.c -laptx100 -lasound $(CFLAGS)

dtstimecode: src/dtstimecode.c $(DEPS)
	gcc -o dtstimecode src/dtstimecode.c $(CFLAGS)

clean:
	rm -f dtsplayback
	rm -f dtstimecode