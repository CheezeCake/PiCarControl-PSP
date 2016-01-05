#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pspkernel.h>
#include <pspctrl.h>

#include "control.h"

#define JOYSTICK_NEUTRAL_RANGE_FROM 111
#define JOYSTICK_NEUTRAL_RANGE_TO 144

#define MOTION_RANGE JOYSTICK_NEUTRAL_RANGE_FROM

#define JOYSTICK_CENTER 128

#define PREFIX 0
#define X 1
#define Y 2

static bool previous_state_neutral = true;
static int sock = -1;
static SceUID mbxId = -1;
static SceUID worker_thread = -1;
static SceKernelMsgPacket glob_msg;

static int _inet_pton(const char* src_ip, struct in_addr* dst)
{
	const char delim[] = ".";
	char ip[INET_ADDRSTRLEN] = {0};
	int pos = 3;
	uint32_t val = 0;

	strncpy(ip, src_ip, INET_ADDRSTRLEN - 1);

	for (char* token = strtok(ip, delim); token; token = strtok(NULL, delim)) {
		if (pos < 0)
			return -1;
		val |= (strtoul(token, NULL, 10) << (8 * pos--));
	}

	dst->s_addr = htonl(val);

	return 0;
}

static int control_worker_thread(SceSize args, void* argp)
{
	printf("worker start\n");
	SceKernelMsgPacket* message;

	while (1) {
		s8 p, x, y;
		sceKernelReceiveMbx(mbxId, (void**)&message, NULL);
		p = message->dummy[PREFIX];
		x = message->dummy[X];
		y = message->dummy[Y];

		printf("worker : %c, %d, %d\n", p, x, y);

		send(sock, &p, 1, 0);
		send(sock, &x, 1, 0);
		send(sock, &y, 1, 0);
	}

	return 0;
}

int control_init(const char* ip)
{
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

	struct sockaddr_in sa;
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock == -1) {
		pspDebugScreenPrintf("cannot create socket : %s\n", strerror(errno));
		return 1;
	}

	memset(&sa, 0, sizeof(sa));

	sa.sin_family = AF_INET;
	sa.sin_port = htons(1100);
	_inet_pton(ip, &sa.sin_addr);

	if (connect(sock, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
		pspDebugScreenPrintf("connect failed : %s\n", strerror(errno));
		control_term();
		return 1;
	}


	if ((mbxId = sceKernelCreateMbx("control_mbx", 0, NULL)) < 0) {
		control_term();
		return 2;
	}

	worker_thread = sceKernelCreateThread("worker_thread", control_worker_thread, 0x11, 0xfa0, 0, NULL);
	if (worker_thread < 0) {
		control_term();
		return 3;
	}

	sceKernelStartThread(worker_thread, 0, NULL);

	return 0;
}

void control_poll_event(void)
{
	SceCtrlData pad;
	sceCtrlReadBufferPositive(&pad, 1);

	if (pad.Lx >= JOYSTICK_NEUTRAL_RANGE_FROM && pad.Lx <= JOYSTICK_NEUTRAL_RANGE_TO &&
			pad.Ly >= JOYSTICK_NEUTRAL_RANGE_FROM && pad.Ly <= JOYSTICK_NEUTRAL_RANGE_TO) {
			if (!previous_state_neutral) {
				previous_state_neutral = true;

				// send 0, 0
				glob_msg.dummy[PREFIX] = 'm';
				glob_msg.dummy[X] = 0;
				glob_msg.dummy[Y] = 0;
				/* sceKernelSendMbx(mbxId, &glob_msg); */
				send(sock, &glob_msg.dummy[PREFIX], 1, 0);
				send(sock, &glob_msg.dummy[X], 1, 0);
				send(sock, &glob_msg.dummy[Y], 1, 0);
			}
	}
	else {
		previous_state_neutral = false;

		s8 x = pad.Lx - ((pad.Lx > JOYSTICK_NEUTRAL_RANGE_TO)
				? JOYSTICK_NEUTRAL_RANGE_TO : JOYSTICK_NEUTRAL_RANGE_FROM);
		s8 y = pad.Ly - ((pad.Ly > JOYSTICK_NEUTRAL_RANGE_TO)
				? JOYSTICK_NEUTRAL_RANGE_TO : JOYSTICK_NEUTRAL_RANGE_FROM);

		x = (x * 100) / MOTION_RANGE;
		y = (y * 100) / MOTION_RANGE;

		glob_msg.dummy[PREFIX] = 'm';
		glob_msg.dummy[X] = x;
		glob_msg.dummy[Y] = y;
		/* sceKernelSendMbx(mbxId, &glob_msg); */
		send(sock, &glob_msg.dummy[PREFIX], 1, 0);
		send(sock, &glob_msg.dummy[X], 1, 0);
		send(sock, &glob_msg.dummy[Y], 1, 0);
	}

	const int cv = 300;
	if (pad.Buttons & PSP_CTRL_LEFT) {
		glob_msg.dummy[PREFIX] = 'c';
		glob_msg.dummy[X] = cv;
		glob_msg.dummy[Y] = 0;

		send(sock, &glob_msg.dummy[PREFIX], 1, 0);
		send(sock, &glob_msg.dummy[X], 1, 0);
		send(sock, &glob_msg.dummy[Y], 1, 0);
	}
	else if (pad.Buttons & PSP_CTRL_RIGHT) {
		glob_msg.dummy[PREFIX] = 'c';
		glob_msg.dummy[X] = -cv;
		glob_msg.dummy[Y] = 0;

		send(sock, &glob_msg.dummy[PREFIX], 1, 0);
		send(sock, &glob_msg.dummy[X], 1, 0);
		send(sock, &glob_msg.dummy[Y], 1, 0);
	}
	else if (pad.Buttons & PSP_CTRL_UP) {
		glob_msg.dummy[PREFIX] = 'c';
		glob_msg.dummy[X] = 0;
		glob_msg.dummy[Y] = cv;

		send(sock, &glob_msg.dummy[PREFIX], 1, 0);
		send(sock, &glob_msg.dummy[X], 1, 0);
		send(sock, &glob_msg.dummy[Y], 1, 0);
	}
	else if (pad.Buttons & PSP_CTRL_DOWN) {
		glob_msg.dummy[PREFIX] = 'c';
		glob_msg.dummy[X] = 0;
		glob_msg.dummy[Y] = -cv;

		send(sock, &glob_msg.dummy[PREFIX], 1, 0);
		send(sock, &glob_msg.dummy[X], 1, 0);
		send(sock, &glob_msg.dummy[Y], 1, 0);
	}
}

void control_term(void)
{
	if (worker_thread != -1) {
		sceKernelTerminateDeleteThread(worker_thread);
		worker_thread = -1;
	}

	if (mbxId != -1) {
		sceKernelDeleteMbx(mbxId);
		mbxId = -1;
	}

	if (sock != -1) {
		shutdown(sock, SHUT_RDWR);
		close(sock);
		sock = -1;
	}
}
