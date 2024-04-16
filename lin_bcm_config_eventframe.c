/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
/* Copyright (C) 2024 hexDEV GmbH - https://hexdev.de */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <errno.h>
#include <linux/can/bcm.h>

#ifndef RX_LIN_SETUP
#define RX_LIN_SETUP 13
#endif

#ifndef RX_LIN_DELETE
#define RX_LIN_DELETE 14
#endif

#ifndef LIN_EVENT_FRAME
#define LIN_EVENT_FRAME     0x1000
#endif

#ifndef LIN_ENHANCED_CKSUM_FLAG
#define LIN_ENHANCED_CKSUM_FLAG	0x00000100U
#endif

#define LINBUS_MAX_ID	63

struct lin_rx_cfg {
	struct bcm_msg_head msg_head;
	struct canfd_frame frame;
};

int send_msg(int s, struct lin_rx_cfg *msg)
{
	ssize_t ret;
	do {
		ret = write(s, msg, sizeof(struct lin_rx_cfg));
		if (ret < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS) {
				// Buffer might be temporarily full, wait a bit and try again
				usleep(100);
				continue;
			} else {
				perror("write");
				return 1;
			}
		}
		break;
	} while (1);

	return 0;
}

int main(int argc, char *argv[])
{
	int s; // Socket
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct lin_rx_cfg msg;
	int ret;

	// Check command line arguments
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <can_interface> [--only-deactivate]\n", argv[0]);
		return 1;
	}

	// Open a BCM socket
	if ((s = socket(PF_CAN, SOCK_DGRAM, CAN_BCM)) < 0) {
		perror("socket");
		return 1;
	}

	strcpy(ifr.ifr_name, argv[1]);
	if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
		perror("SIOCGIFINDEX");
		close(s);
		return 1;
	}

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		close(s);
		return 1;
	}

	// Deactivate all possible preconfigure frames and event IDs
	for (int i = 0; i <= LINBUS_MAX_ID; i++) {
		memset(&msg, 0, sizeof(msg));
		msg.msg_head.opcode = RX_LIN_DELETE;
		msg.msg_head.flags = CAN_FD_FRAME;
		msg.msg_head.nframes = 1;
		msg.frame.can_id = i;
		ret = send_msg(s, &msg);
		if (ret)
			return EXIT_FAILURE;
		//printf("Deactivation no %i complete.\n", i);
	}

	// Check if "--only-deactivate" is set
	if (argc > 2 && strcmp(argv[2], "--only-deactivate") == 0) {
		printf("Deactivation complete, exiting.\n");
		close(s);
		return EXIT_SUCCESS;
	}

	// Configure a LIN answer (rx): Preload a rx LIN answer on ID 0x14
	memset(&msg, 0, sizeof(msg));
	msg.msg_head.opcode = RX_LIN_SETUP;
	msg.msg_head.flags = CAN_FD_FRAME;
	msg.msg_head.nframes = 1;
	msg.frame.can_id = 0x14;
	// preload data to respond with
	msg.frame.len = 6;
	msg.frame.data[0] = 0xaa;
	msg.frame.data[1] = 0xbb;
	msg.frame.data[2] = 0xcc;
	msg.frame.data[3] = 0xdd;
	msg.frame.data[4] = 0xee;
	msg.frame.data[5] = 0xff;
	ret = send_msg(s, &msg);
	if (ret)
		return EXIT_FAILURE;

	// Configure LIN_EVENT_FRAME on id 0x8 associating (only new) data with event id 0x14
	memset(&msg, 0, sizeof(msg));
	msg.msg_head.opcode = RX_LIN_SETUP;
	msg.msg_head.flags = CAN_FD_FRAME | LIN_EVENT_FRAME;
	msg.msg_head.can_id = 0x14;
	msg.msg_head.nframes = 1;
	msg.frame.can_id = 8 /*| LIN_ENHANCED_CKSUM_FLAG */;
	ret = send_msg(s, &msg);
	if (ret)
		return EXIT_FAILURE;

	printf("BCM RX_LIN_SETUP messages sent successfully.\n");

	close(s);
	return EXIT_SUCCESS;
}
