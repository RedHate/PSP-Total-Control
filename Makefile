TARGET = main
OBJS = lightmap.o main.o

GAME_CAT = MG
# Define to build this as a prx (instead of a static elf)
BUILD_PRX=1
# Define the name of our custom exports (minus the .exp extension)
PRX_EXPORTS=exports.exp
#PSP_FW_VERSION = 500

INCDIR =
CFLAGS = -G0 -Wall -O2
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
LIBS= -lintrafont_psp -lpspreg -lpspgum -lpspgu -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = SysDiag

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

lightmap.o : lightmap.raw
	bin2o -i lightmap.raw lightmap.o lightmap

copy:
	rm -rf F:\PSP\GAME\registry
	mkdir F:\PSP\GAME\registry
	cp -Rp EBOOT.PBP F:\PSP\GAME\registry\EBOOT.PBP
	
copy_f:
	rm -rf F:\PSP\GAME\registry
	mkdir F:\PSP\GAME\registry
	cp -Rp EBOOT.PBP F:\PSP\GAME\registry\EBOOT.PBP
