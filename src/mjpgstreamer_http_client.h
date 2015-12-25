#ifndef _MJPGSTREAMER_HTTP_CLIENT_
#define _MJPGSTREAMER_HTTP_CLIENT_

#include <stddef.h>

struct Mjpgstreamer_connection
{
	int template_id;
	int connection_id;
	int request_id;
};

int mjpgstreamer_http_client_connect(struct Mjpgstreamer_connection* con,
		char* ip, unsigned short port, char* path);
int mjpgstreamer_http_client_read_frame(const struct Mjpgstreamer_connection* con,
		char** jpg_frame, size_t* jpg_frame_size);
void mjpgstreamer_http_client_disconnect(const struct Mjpgstreamer_connection* con);


#endif
