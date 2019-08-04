#
#	Makefile for the IT-HE support library
#


#Debug = 1	# Uncomment for debug version

all: library

CC                = gcc
ifdef Debug
CFLAGS            = -I. -g -O0 -DALLEGRO -DITHE_DEBUG
library          = libithed.a
else
CFLAGS            = -I. -g -O2 -DALLEGRO
library          = libithe.a
endif

Librarian         = ar


Object_files   = debug.o doslib.o fs.o memory.o strlib.o


library: $(Object_files)
	-rm -f $(library)
	$(Librarian) rcs $(library) $^

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cc
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.asm
	$(ASM) -f coff $< -o $@

clean:
	rm -f *.o $(library)
