#include <pspkernel.h>
#include <pspdebug.h>
#include "wifi.h"

PSP_MODULE_INFO("PiCarControl", PSP_MODULE_USER, 1, 0);
PSP_HEAP_SIZE_KB(5 * 1024);

int exit_callback(int arg1, int arg2, void* common)
{
	sceKernelExitGame();
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
	if (err != 0) {
		pspDebugScreenPrintf("wifi_init() failed : %d\n", err);
	}
	else {
		wifi_connect_ssid("PiCar");
		wifi_disconnect();
		sceKernelDelayThread(5 * 1000 * 1000);
		wifi_term();
	}

	sceKernelSleepThread();

	return 0;
}
