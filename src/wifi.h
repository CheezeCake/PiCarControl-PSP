#ifndef _WIFI_H_
#define _WIFI_H_

#include <stdbool.h>

int wifi_init(void);
int wifi_term(void);
int wifi_connect_ssid(const char* ssid);
int wifi_connect_confid(int confid);
bool wifi_is_connected(void);
int wifi_disconnect(void);

#endif
