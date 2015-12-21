PSPSDK = $(shell psp-config --pspsdk-path)

CFLAGS = -std=c99
LIBS = -lpspwlan -lpsphttp

TARGET = PiCarControl
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PiCar Control

BUILD_PRX = 1
PSP_FW_VERSION = 660

OBJS = main.o wifi.o

include $(PSPSDK)/lib/build.mak
