#include <stdlib.h>
#include <string.h>
#include <pspkernel.h>
#include <psphttp.h>
#include "mjpgstreamer_http_client.h"

static const char soi_marker[2] = { 0xff, 0xd8 };

int mjpgstreamer_http_client_connect(struct Mjpgstreamer_connection* con,
		char* ip, unsigned short port, char* path)
{
	con->template_id = sceHttpCreateTemplate("psp", 1, 0);
	if (con->template_id < 0) {
		pspDebugScreenPrintf("sceHttpCreateTemplate() failed\n");
		return 1;
	}

	con->connection_id = sceHttpCreateConnection(con->template_id, ip, "http", port, 0);
	if (con->connection_id < 0) {
		pspDebugScreenPrintf("sceHttpCreateConnection() failed\n");
		return 1;
	}

	con->request_id = sceHttpCreateRequest(con->connection_id, PSP_HTTP_METHOD_GET, path, 0);
	if (con->request_id < 0) {
		pspDebugScreenPrintf("sceHttpCreateRequest() failed\n");
		return 1;
	}

	if (sceHttpSendRequest(con->request_id, NULL, 0) != 0) {
		pspDebugScreenPrintf("sceHttpSendRequest() failed\n");
		return 1;
	}

	return 0;
}

static int parse_content_length(const char* header, size_t size)
{
	const char content_length[] = "Content-Length: ";
	char* pos = strstr(header, content_length);
	int content_length_value = 0;

	if (pos) {
		pos += sizeof(content_length) - 1;
		if (pos < header + size)
			content_length_value = strtol(pos, NULL, 10);
	}

	return content_length_value;
}

static int read_header(int request_id, char* buffer,
		size_t size, size_t* bytes_read)
{
	int marker_index = 0;
	*bytes_read = 0;

	while (marker_index != 2 &&
			*bytes_read + 1 < size &&
			sceHttpReadData(request_id, &buffer[*bytes_read], 1) > 0) {
		if (buffer[*bytes_read] == soi_marker[marker_index])
			++marker_index;
		else
			marker_index = 0;

		++(*bytes_read);
	}

	buffer[*bytes_read] = '\0';

	return (marker_index == 2) ? *bytes_read : -1;
}

int mjpgstreamer_http_client_read_frame(const struct Mjpgstreamer_connection* con,
		char** jpg_frame, size_t* jpg_frame_size)
{
	const int BUFFER_SIZE = 512;
	char buffer[BUFFER_SIZE];
	size_t length;
	int n;

	if (!jpg_frame)
		return 1;

	n = read_header(con->request_id, buffer, BUFFER_SIZE, &length);

	int content_length = parse_content_length(buffer, length);

	if (!*jpg_frame || *jpg_frame_size == 0)
		*jpg_frame = malloc(content_length);
	else if (*jpg_frame_size < content_length)
		*jpg_frame = realloc(*jpg_frame, content_length);

	if (!*jpg_frame)
		return 1;

	// copy the first 2 bytes of the image we read when reading the header
	memcpy(*jpg_frame, soi_marker, 2);
	content_length -= 2;

	int index = 2;
	n = 1;
	// TODO: read by block
	while (n > 0 && index < content_length) {
		n = sceHttpReadData(con->request_id, *jpg_frame + index, 1);
		index += n;
	}

	*jpg_frame_size = index;

	return (index < content_length);
}

void mjpgstreamer_http_client_disconnect(const struct Mjpgstreamer_connection* con)
{
	sceHttpDeleteRequest(con->request_id);
	sceHttpDeleteConnection(con->connection_id);
	sceHttpDeleteTemplate(con->template_id);
}
