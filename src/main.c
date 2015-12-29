#include <stdbool.h>

#include <pspkernel.h>
#include <pspdebug.h>

#include "wifi.h"
#include "mjpgstreamer_http_client.h"
#include "render.h"
#include "jpeg_frame.h"

PSP_MODULE_INFO("PiCarControl", PSP_MODULE_USER, 1, 0);
PSP_HEAP_SIZE_KB(5 * 1024);

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

		struct Mjpgstreamer_connection con;
		if (mjpgstreamer_http_client_connect(&con, "192.168.1.10", 8080, "/?action=stream") == 0) {
			struct Jpeg_frame frame;
			jpeg_frame_init(&frame);

			if (render_init() != 0)
				done = true;

			while (!done && mjpgstreamer_http_client_read_frame(&con, &frame) == 0)
				render_jpeg_frame(&frame);

			render_term();
			jpeg_frame_destroy(&frame);
		}

		mjpgstreamer_http_client_disconnect(&con);

		wifi_disconnect();
		sceKernelDelayThread(2 * 1000 * 1000);
		wifi_term();
	}
	else {
		pspDebugScreenPrintf("wifi_init() failed : %d\n", err);
	}

	/* sceKernelSleepThread(); */
	sceKernelDelayThread(2 * 1000 * 1000);
	sceKernelExitGame();

	return 0;
}
