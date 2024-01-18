/**
 * From https://github.com/linux-can/can-tests/blob/master/raw/canecho.c.
 * 
 * Reads can messages from can0 and prints them to stdout.
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>

#include <linux/can.h>

extern int optind, opterr, optopt;

static int	s = -1;
static int	running = 1;

void print_usage(char *prg)
{
	fprintf(stderr, "Usage: %s [can-interface]\n", prg);
}

void sigterm(int signo)
{
	printf("got signal %d\n", signo);
	running = 0;
}

/**
 * Echo received CAN messages.
*/
int main(int argc, char **argv)
{
	int family = PF_CAN, type = SOCK_RAW, proto = CAN_RAW;
	int opt;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame frame;
	int nbytes, i;
	int verbose = 1;

	signal(SIGTERM, sigterm);
	signal(SIGHUP, sigterm);

	printf("interface = %s, family = %d, type = %d, proto = %d\n",
	       argv[optind], family, type, proto);
	if ((s = socket(family, type, proto)) < 0) {
		perror("socket");
		return 1;
	}

	addr.can_family = family;
	strcpy(ifr.ifr_name, "can0");
	ioctl(s, SIOCGIFINDEX, &ifr);
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}

	while (running) {
		if ((nbytes = read(s, &frame, sizeof(frame))) < 0) {
			perror("read");
			return 1;
		}
		if (verbose) {
			printf("%03X: ", frame.can_id & CAN_EFF_MASK);
			if (frame.can_id & CAN_RTR_FLAG) {
                abort(); // crash
				printf("remote request");
			} else {
				printf("[%d]", frame.can_dlc);
				for (i = 0; i < frame.can_dlc; i++) {
					printf(" %02X", frame.data[i]);
				}
			}
			printf("\n");
		}
		frame.can_id++;
		if (write(s, &frame, sizeof(frame)) < 0) {
			perror("write");
			exit(1);
		}
	}

	if (close(s) < 0) {
		perror("Close");
		return 1;
	}

	return 0;
}