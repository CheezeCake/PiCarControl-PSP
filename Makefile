PSPSDK = $(shell psp-config --pspsdk-path)

CFLAGS = -std=c99 -pipe #-g -ggdb
LIBS = -lpspwlan -lpsphttp -lpspgu -lpspaudio -lpsphprm -lpspirkeyb -lpspvfpu \
	   -lpsprtc -lpsppower -lpspjpeg -lpspgu

TARGET = PiCarControl
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PiCar Control

BUILD_PRX = 1
PSP_FW_VERSION = 660

OBJS = src/main.o src/wifi.o src/mjpgstreamer_http_client.o src/render.o \
	   src/vram.o

include $(PSPSDK)/lib/build.mak
