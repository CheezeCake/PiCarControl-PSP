#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "wifi.h"
#include "mjpgstreamer_http_client.h"

PSP_MODULE_INFO("PiCarControl", PSP_MODULE_USER, 1, 0);
PSP_HEAP_SIZE_KB(7 * 1024);

bool done = false;

int exit_callback(int arg1, int arg2, void* common)
{
	if (done)
		sceKernelExitGame();
	else
		done = true;

	return 0;
}

int callback_thread(SceSize args, void* argp)
{
	int cbid;
	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

int setup_callbacks(void)
{
	int thid = 0;
	thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xfa0, 0, NULL);
	if (thid >= 0)
		sceKernelStartThread(thid, 0, 0);

	return thid;
}

int main(void)
{
	pspDebugScreenInit();

	setup_callbacks();

	int err = wifi_init();
	if (err == 0) {
		wifi_connect_ssid("PiCar");

		SDL_Init(SDL_INIT_VIDEO);
		IMG_Init(IMG_INIT_JPG);
		SDL_Surface* screen = SDL_SetVideoMode(480, 272, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);

		struct Mjpgstreamer_connection con;
		if (mjpgstreamer_http_client_connect(&con, "192.168.1.10", 8080, "/?action=stream") == 0) {
			char* frame = NULL;
			size_t size = 0;

			while (!done && mjpgstreamer_http_client_read_frame(&con, &frame, &size) == 0) {
				SDL_RWops* img = SDL_RWFromConstMem(frame, size);

				if (img) {
					SDL_Surface* img_surface = IMG_LoadJPG_RW(img);

					if (img_surface) {
						SDL_BlitSurface(img_surface, NULL, screen, &(SDL_Rect) { 0, 0 });
						SDL_Flip(screen);
					}
					else {
						printf("loadjpg failed : %s | 0x%x %d\n", IMG_GetError(), frame, size);
					}

					SDL_FreeSurface(img_surface);
				}
				else {
					printf("rwfromconstmem failed :  | 0x%x %d\n", SDL_GetError(), frame, size);
				}
			}

			free(frame);
		}

		mjpgstreamer_http_client_disconnect(&con);

		wifi_disconnect();
		sceKernelDelayThread(2 * 1000 * 1000);
		wifi_term();

		IMG_Quit();
		SDL_Quit();
	}
	else {
		pspDebugScreenPrintf("wifi_init() failed : %d\n", err);
	}

	/* sceKernelSleepThread(); */
	sceKernelDelayThread(2 * 1000 * 1000);
	sceKernelExitGame();

	return 0;
}
