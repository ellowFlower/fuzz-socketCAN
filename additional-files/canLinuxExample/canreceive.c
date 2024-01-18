/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
/*
 * from https://github.com/linux-can/can-tests/blob/master/raw/tst-raw-filter.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#define MAXFILTERS 32


int containsByte(const struct can_frame *frame, unsigned char byte) {
    for (int i = 0; i < frame->len; i++) {
        if (frame->data[i] == byte) {
            return 1;
        }
    }
    return 0;
}

int accessArrayOnIndex(const struct can_frame *frame) {
	int myNumbers[4] = {25, 50, 75, 100};
	printf("frame->data[0]: %d\n", frame->data[0]);
	printf("frame->data[1]: %d\n", frame->data[1]);
	printf("frame->data[2]: %d\n", frame->data[2]);
	int index = frame->data[0] + frame->data[1] - frame->data[2];
	int value = myNumbers[index];
	return value;
}

int main(int argc, char **argv)
{
	int s;
	struct sockaddr_can addr;
	struct can_filter rfilter[MAXFILTERS];
	struct can_frame frame;
	int nbytes, i;
	struct ifreq ifr;
	char *ifname = "any";
	int ifindex;
	int opt;
	int peek = 0;
	int nfilters = 0;
	int deflt = 0;


	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("socket");
		return 1;
	}

	if (strcmp(ifname, "any") == 0)
		ifindex = 0;
	else {
		strcpy(ifr.ifr_name, ifname);
		ioctl(s, SIOCGIFINDEX, &ifr);
		ifindex = ifr.ifr_ifindex;
	}

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}

	while (1) {
		socklen_t len = sizeof(addr);
		int flags;

		if (peek && peek--)
			flags = MSG_PEEK;
		else
			flags = 0;
		nbytes = recvfrom(s, &frame, sizeof(struct can_frame),
				  flags, (struct sockaddr*)&addr, &len);

		if (nbytes < 0) {
			perror("read");
	
			return 1;
		} else if (nbytes < sizeof(struct can_frame)) {
			fprintf(stderr, "read: incomplete CAN frame from iface %d\n",
				addr.can_ifindex);
			
			return 1;
		} else {
			ifr.ifr_ifindex = addr.can_ifindex;
			ioctl(s, SIOCGIFNAME, &ifr);
			printf(" %-5s ", ifr.ifr_name);
			if (frame.can_id & CAN_EFF_FLAG)
				printf("%8X  ", frame.can_id & CAN_EFF_MASK);
			else
				printf("%3X  ", frame.can_id & CAN_SFF_MASK);

			printf("[%d] ", frame.can_dlc);

			for (i = 0; i < frame.can_dlc; i++) {
				printf("%02X ", frame.data[i]);
			}
			if (frame.can_id & CAN_RTR_FLAG)
				printf("remote request");
			if (flags & MSG_PEEK)
				printf(" (MSG_PEEK)");
			printf("\n");
			fflush(stdout);
		}


		int result = accessArrayOnIndex(&frame);
		printf("Result: %d\n", result);
	}

	close(s);

	return 0;
}