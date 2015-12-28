PSPSDK = $(shell psp-config --pspsdk-path)

CFLAGS = -std=c99 #-g -ggdb
LIBS = -lSDL -lSDL_image -lGL -ljpeg -lpng -lz -lm -lpspwlan -lpsphttp -lpspgu \
	   -lpspaudio -lpsphprm -lpspirkeyb -lpspvfpu -lpsprtc -lpsppower -lpspjpeg

TARGET = PiCarControl
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PiCar Control

BUILD_PRX = 1
PSP_FW_VERSION = 660

OBJS = src/main.o src/wifi.o src/mjpgstreamer_http_client.o

include $(PSPSDK)/lib/build.mak
