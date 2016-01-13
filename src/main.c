#include <stdio.h>
#include <stdbool.h>

#include <pspkernel.h>
#include <pspdebug.h>

#include "wifi.h"
#include "mjpgstreamer_http_client.h"
#include "render.h"
#include "jpeg_frame.h"
#include "control.h"

#define PICAR_IP "192.168.1.10"

PSP_MODULE_INFO("PiCarControl", PSP_MODULE_USER, 1, 0);
PSP_HEAP_SIZE_KB(5 * 1024);

static bool done = false;

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
		sceKernelStartThread(thid, 0, NULL);

	return thid;
}

void run(void)
{
	char ip[] = PICAR_IP;
	struct Mjpgstreamer_connection con;
	memset(&con, -1, sizeof(struct Mjpgstreamer_connection));

	if (mjpgstreamer_http_client_connect(&con, ip, 8080, "/?action=stream") == 0) {
		struct Jpeg_frame frame;
		jpeg_frame_init(&frame);

		if (control_init(ip) != 0) {
			pspDebugScreenPrintf("control_init() failed\n");
		}
		else if (render_init() != 0) {
			pspDebugScreenPrintf("render_init() failed\n");
			done = true;
		}

		while (!done && mjpgstreamer_http_client_read_frame(&con, &frame) == 0) {
			render_jpeg_frame(&frame);
			control_poll_event();
		}

		control_term();
		printf("control_term done\n");
		render_term();
		printf("render_term done\n");
		jpeg_frame_destroy(&frame);
		printf("jpeg_frame_destroy done\n");
	}

	mjpgstreamer_http_client_disconnect(&con);
	printf("mjpgstreamer_http_client_disconnect done\n");
}

int main(void)
{
	pspDebugScreenInit();

	setup_callbacks();

	if (wifi_init() != 0) {
		pspDebugScreenPrintf("wifi_init() failed\n");
		sceKernelExitGame();
		return 1;
	}

	if (wifi_connect_ssid("PiCar") != 0) {
		pspDebugScreenPrintf("wifi_connect_failed\n");
	}
	else {
		run();
		sceKernelDelayThread(2 * 1000 * 1000);
		wifi_disconnect();
		printf("wifi_disconnect done\n");
	}

	sceKernelDelayThread(3 * 1000 * 1000);
	wifi_term();
	printf("wifi_term done\n");

	sceKernelDelayThread(1 * 1000 * 1000);
	sceKernelExitGame();

	return 0;
}
