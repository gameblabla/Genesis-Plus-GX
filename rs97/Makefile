#
# Makefile for RS97
#

CHAINPREFIX=/opt/rs97-toolchain
CROSS_COMPILE=$(CHAINPREFIX)/usr/bin/mipsel-linux-
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
STRIP = $(CROSS_COMPILE)strip
SYSROOT     := $(CHAINPREFIX)/usr/mipsel-buildroot-linux-uclibc/sysroot
SDL_CFLAGS  := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS    := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

OUTPUTNAME = gen.elf

DEFINES = -DLSB_FIRST -DNOLIMIT -std=gnu99
INCLUDES = -I. -I../source -I../source/z80 -I../source/m68k -I../source/sound -I../source/input_hw -I../source/cart_hw -I../source/cart_hw/svp -I../source/ntsc

OPT_FLAGS  = -O2 -fomit-frame-pointer -fdata-sections -ffunction-sections -mhard-float -mips32 -mno-mips16

CFLAGS = $(DEFINES) $(INCLUDES) $(OPT_FLAGS) $(SDL_CFLAGS)
CXXFLAGS = $(DEFINES) $(INCLUDES) $(OPT_FLAGS)
LDFLAGS = $(SDL_LIBS) -lm -lz -Wl,--as-needed -Wl,--gc-sections -flto -s

# Redream (main engine)
OBJECTS	=       ../source/z80/z80.o	

OBJECTS	+=     	../source/m68k/m68kcpu.o \
	../source/m68k/m68kops.o 

OBJECTS	+=     	../source/genesis.o	 \
		../source/vdp_ctrl.o	 \
		../source/vdp_render.o \
		../source/system.o     \
		../source/io_ctrl.o	 \
		../source/mem68k.o	 \
		../source/memz80.o	 \
		../source/membnk.o	 \
		../source/state.o     

OBJECTS	+=      ../source/input_hw/input.o	 \
		../source/input_hw/gamepad.o	 \
		../source/input_hw/lightgun.o	 \
		../source/input_hw/mouse.o	 \
		../source/input_hw/activator.o	 \
		../source/input_hw/xe_a1p.o	 \
		../source/input_hw/teamplayer.o \
		../source/input_hw/paddle.o	 \
		../source/input_hw/sportspad.o 


OBJECTS	+=      ../source/sound/sound.o	 \
		 ../source/sound/sn76489.o    \
		 ../source/sound/ym2612.o   \
		 ../source/sound/ym2413.o 

OBJECTS	+=	../source/sound/Fir_Resampler.o
OBJECTS	+=	../source/sound/blip.o 

OBJECTS	+=	../source/sound/eq.o \

OBJECTS	+=      ../source/cart_hw/sram.o      \
		../source/cart_hw/eeprom.o    \
		../source/cart_hw/svp/svp.o	\
		../source/cart_hw/svp/ssp16.o	\
		../source/cart_hw/ggenie.o	\
		../source/cart_hw/areplay.o	\
		../source/cart_hw/md_cart.o	\
		../source/cart_hw/sms_cart.o	

OBJECTS	+=	main.o	\
		config.o	\
		error.o	\
		unzip.o     \
		fileio.o	\
		rs97_dma.o	\
		../source/loadrom.o	
		
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< 

all: executable

executable : $(OBJECTS)
	$(CC) -o $(OUTPUTNAME) $(OBJECTS) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(OBJECTS) $(OUTPUTNAME)
