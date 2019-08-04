#
# Makefile for IRE, BeOS version
#

all: ire ed chknpc 

# Use the C or C++ compiler
CC                = gcc
LD		  = gcc -Llibs/beos
CFLAGS            = -I. -Ilibs/beos/allegro -Ijpeg/dos -g -Wunused -O6 -DALLEGRO -DBGUI -DUSE_SDLSOUND
Xopts		  = `allegro-config --libs`

Lib_files         = -lalleg -lithe -ljpeg -lbgui 

# Include the makefile core

include makefile.all


# BeOS-dependent components

be_obj = darken_a.o audio/sdl.o


#resize_obj = resize.o doslib.o memory.o dummy2a.o endian.o

# Makefile rules

ire: $(ire_obj) $(be_obj)
	$(LD) $(Xopts) $(ire_obj) $(be_obj) $(Lib_files) -o ire

ed: $(ed_obj) $(be_obj)
	$(LD) $(Xopts) $(ed_obj) $(be_obj) $(Lib_files) -o ed

resedit: $(res_obj) $(be_obj)
	$(LD) $(Xopts) $(res_obj) $(be_obj) $(Lib_files) -o res

resize: $(resize_obj)
	$(LD) $(Xopts) $(resize_obj) $(Lib_files) -o tools/resize_map

chknpc: $(chknpc_obj)
	$(LD) $(Xopts) $(chknpc_obj) $(Lib_files) -o tools/chknpc

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cc
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.S
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.asm
	nasm -f elf $< -o $@

clean:
	-rm -f $(ed_obj) $(ire_obj) $(res_obj) $(build_obj) $(resize_obj)
	-rm -f $(lin_obj) $(chknpc_obj)
	-rm -f ire
	-rm -f ed
	-rm -f res
	-rm -f tools/build
	-rm -f tools/chknpc
	-rm -f tools/resize_map
	-rm -f bootlog.* game.cfg
