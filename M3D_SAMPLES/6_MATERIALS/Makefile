PSPSDK=$(shell psp-config --pspsdk-path)
TARGET = MATERIALS
OBJS = main.o

INCDIR = $(PSPSDK)/include
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR = $(PSPSDK)/lib
LDFLAGS = 

LIBS = -lM3D -lbulletpsp -lpng -lz -lm -lstdc++\
		-lpspjpeg -lpspmpeg -lpspmpegbase -lpsphprm -lpspsdk -lpspctrl -lpspumd -lpsprtc -lpsppower -lpspgu -lpspgum\
		-lpspaudiolib -lpspaudio -lpspaudiocodec -lpsphttp -lpspssl -lpspwlan -lpspnet_adhocmatching\
		-lpspnet_adhoc -lpspnet_adhocctl  

PSP_FW_VERSION = 660

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = MATERIALS
PSP_EBOOT_ICON = ../00_ICONS/ICON0.PNG


include $(PSPSDK)/lib/build.mak

