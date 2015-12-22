#include <string.h>
#include <pspkernel.h>
#include <psputility_netparam.h>
#include <psputility_netmodules.h>
#include <pspnet.h>
#include <pspnet_apctl.h>
#include <pspnet_inet.h>
#include <pspnet_resolver.h>
#include <psphttp.h>
#include <pspwlan.h>
#include "wifi.h"

static int psp_load_network_modules(void)
{
	if (sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON)) {
		pspDebugScreenPrintf("sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON) failed\n");
		return 1;
	}
	if (sceUtilityLoadNetModule(PSP_NET_MODULE_INET)) {
		pspDebugScreenPrintf("sceUtilityLoadNetModule(PSP_NET_MODULE_INET) failed\n");
		return 2;
	}
	if (sceUtilityLoadNetModule(PSP_NET_MODULE_PARSEURI)) {
		pspDebugScreenPrintf("sceUtilityLoadNetModule(PSP_NET_MODULE_PARSEURI) failed\n");
		return 3;
	}
	if (sceUtilityLoadNetModule(PSP_NET_MODULE_PARSEHTTP)) {
		pspDebugScreenPrintf("sceUtilityLoadNetModule(PSP_NET_MODULE_PARSEHTTP) failed\n");
		return 4;
	}
	// PARSEURI and PARSEHTTP must be loaded before HTTP
	if (sceUtilityLoadNetModule(PSP_NET_MODULE_HTTP)) {
		pspDebugScreenPrintf("sceUtilityLoadNetModule(PSP_NET_MODULE_HTTP) failed\n");
		return 5;
	}

	return 0;
}

static int psp_unload_network_modules(void)
{
	sceUtilityUnloadNetModule(PSP_NET_MODULE_COMMON);
	sceUtilityUnloadNetModule(PSP_NET_MODULE_INET);
	sceUtilityUnloadNetModule(PSP_NET_MODULE_PARSEURI);
	sceUtilityUnloadNetModule(PSP_NET_MODULE_PARSEHTTP);
	sceUtilityUnloadNetModule(PSP_NET_MODULE_HTTP);

	return 0;
}

static int psp_user_networking_init(void)
{
	if (sceNetInit(0x20000, 0x20, 0x1000, 0x20, 0x1000) != 0)
		return 1;
	if (sceNetInetInit() != 0)
		return 2;
	if (sceNetResolverInit() != 0)
		return 3;
	if (sceNetApctlInit(0x1400, 0x18) != 0)
		return 4;
	if (sceHttpInit(0x20000) != 0)
		return 5;

	return 0;
}

static int psp_user_networking_term(void)
{
	sceNetTerm();
	sceNetInetTerm();
	sceNetResolverTerm();
	sceNetApctlTerm();
	sceHttpEnd();

	return 0;
}

int wifi_init(void)
{
	int err = psp_load_network_modules();
	if (err != 0)
		return err;

	return psp_user_networking_init();
}

int wifi_term(void)
{
	if (wifi_is_connected())
		wifi_disconnect();

	psp_user_networking_term();
	psp_unload_network_modules();

	return 0;
}

static inline bool psp_wlan_is_on(void)
{
	return (sceWlanDevIsPowerOn() == 1 && sceWlanGetSwitchState() == 1);
}

int wifi_connect_ssid(const char* ssid)
{
	int confid = -1;
	int iNetIndex = 1; // skip index 0
	netData configData;

	pspDebugScreenPrintf("Searching for wifi configurations...\n");
	while (confid == -1 && sceUtilityCheckNetParam(iNetIndex) == 0) {
		sceUtilityGetNetParam(iNetIndex, PSP_NETPARAM_NAME, &configData);
		pspDebugScreenPrintf("\t%d : %s\n", iNetIndex, configData.asString);
		if (strcmp(ssid, configData.asString) == 0)
			confid = iNetIndex;

		++iNetIndex;
	}

	if (confid == -1) {
		pspDebugScreenPrintf("%d not found !\n");
		return 1;
	}

	return wifi_connect_confid(confid);
}

// TODO: setup apctl event handler to catch wifi errors later on?
int wifi_connect_confid(int confid)
{
	if (!psp_wlan_is_on()) {
		pspDebugScreenPrintf("Wlan is turned off!\n");
		return 1;
	}

	pspDebugScreenPrintf("Connecting to wifi %d...\n", confid);
	if (sceNetApctlConnect(confid) != 0) {
		pspDebugScreenPrintf("sceNetApctlConnect() failed\n");
		return 2;
	}

	int lastState = -1;
	while (1) {
		int state;
		if (sceNetApctlGetState(&state) != 0) {
			pspDebugScreenPrintf("sceNetApctlGetState() failed\n");
			wifi_disconnect();
			return 3;
		}

		if (state != lastState) {
			lastState = state;

			pspDebugScreenPrintf("state = %d\n", state);

			if (state == PSP_NET_APCTL_STATE_JOINING) {
				pspDebugScreenPrintf("joining...\n");
			}
			else if (state == PSP_NET_APCTL_STATE_DISCONNECTED) {
				pspDebugScreenPrintf("disconnected!\n");
				return 4;
			}
		}

		if (state == PSP_NET_APCTL_STATE_GOT_IP ) {
			pspDebugScreenPrintf("connected!\n");
			return 0;
		}

		sceKernelDelayThread(50 * 1000); // 50ms
	}
}

bool wifi_is_connected(void)
{
	int state;
	return (sceNetApctlGetState(&state) < 0) ? false : (state == PSP_NET_APCTL_STATE_GOT_IP);
}

int wifi_disconnect(void)
{
	return sceNetApctlDisconnect();
}
