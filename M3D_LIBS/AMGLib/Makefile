TARGET_LIB = libAMG.a
OBJS = pspDveManager.o AMG_VFPU.o AMG_Multimedia.o AMG_Physics.o AMG_triParticle.o AMG_User.o AMG_3D.o AMG_Texture.o  AMG_Model.o AMG_ModelAnim.o

INCDIR = 
CFLAGS = -O2 -G0 -Wall 
#-Wcast-align
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

AMG_LIBS = -llodepng -lmad -lpspaudiolib -lpspaudiocodec -lpspaudio -lpspmp3 -lbulletpsp -lpspmath -lpsputility -lpspctrl -lpsppower -lpsprtc -lpspgu -lm -lstdc++ -lpspsdk
OSL_LIBS = -losl -lpng -lz -lpsphprm -lpspmpeg -lpspjpeg -lpspmpegbase -lpspaudio -lpspmp3 -lpsputility -lpspctrl -lpsppower -lpsprtc -lpspgu -lm -lpspsdk -lpspaudiocodec

LIBDIR =
LDFLAGS =
LIBS = $(OSL_LIBS)

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
