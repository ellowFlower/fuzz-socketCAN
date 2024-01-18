/**
 * read from a CAN bus and print the received payload using can fd
 * 
 * cansend can0 550##3112233445506778899AABBCCDDEEFF
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#define CAN_INTERFACE "can0"

int containsByte(const struct canfd_frame *frame, unsigned char byte) {
    for (int i = 0; i < frame->len; i++) {
        if (frame->data[i] == byte) {
            return 1;
        }
    }
    return 0;
}

int main() {
    int s, nbytes;
    struct sockaddr_can addr;
    struct canfd_frame frame;
    struct ifreq ifr;
    int enable_fd = 1; // Enable CAN FD frames

    // Create a socket
    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) {
        perror("Socket creation error");
        return -1;
    }

    // Set the socket option to enable CAN FD frames
    if (setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable_fd, sizeof(enable_fd)) < 0) {
        perror("Socket option setting error");
        close(s);
        return -1;
    }

    // Specify the CAN interface
    strcpy(ifr.ifr_name, CAN_INTERFACE);
    ioctl(s, SIOCGIFINDEX, &ifr);

    // Bind the socket to the CAN interface
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Socket binding error");
        close(s);
        return -1;
    }

    printf("Listening for CAN FD frames on %s...\n", CAN_INTERFACE);

    struct can_filter rfilter[1];
	rfilter[0].can_id   = 0x550;
	rfilter[0].can_mask = 0xFF0;
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

    while (1) {
        nbytes = read(s, &frame, sizeof(struct canfd_frame));
        if (nbytes < 0) {
            perror("Socket read error");
            close(s);
            return -1;
        } else if (nbytes < sizeof(struct canfd_frame)) {
            fprintf(stderr, "Incomplete CAN FD frame received\n");
            continue;
        }


		if (frame.can_id == 0x550 
				&& containsByte(&frame, 0x06)
				&& containsByte(&frame, 0xFF)) {
			abort();
		}


        // Print the received payload
        printf("Received payload: ");
        for (int i = 0; i < frame.len; i++) {
            printf("%02X ", frame.data[i]);
        }
        printf("\n");
    }

    close(s);
    return 0;
}
