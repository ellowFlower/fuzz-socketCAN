/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
/*
 * canlogserver.c; commit b8e26388f859d3cc79446b6a7588a9bb90c697a7
 *
 * Copyright (c) 2002-2007 Volkswagen Group Electronic Research
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Volkswagen nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") version 2, in which case the provisions of the
 * GPL apply INSTEAD OF those given above.
 *
 * The provided data structures and external interfaces from this code
 * are not restricted to be used by modules with a GPL compatible license.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * Send feedback to <linux-can@vger.kernel.org>
 *
 */

#include <ctype.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include <errno.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/sockios.h>
#include <signal.h>

#include "lib.h"
#include <netdb.h>
#include <arpa/inet.h>


#define MAXDEV 6 /* change sscanf()'s manually if changed here */
#define ANYDEV "any"
#define ANL "\r\n" /* newline in ASC mode */

#define COMMENTSZ 200
#define BUFSZ (sizeof("(1345212884.318850)") + IFNAMSIZ + 4 + CL_CFSZ + COMMENTSZ) /* for one line in the logfile */

#define DEFPORT 28700

#define CC_DLC_DELIM '_'
#define put_sff_id(buf, id) _put_id(buf, 2, id)
#define put_eff_id(buf, id) _put_id(buf, 7, id)
const char hex_asc_upper1[] = "0123456789ABCDEF";

#define hex_asc_upper_lo(x)	hex_asc_upper1[((x) & 0x0F)]
#define hex_asc_upper_hi(x)	hex_asc_upper1[((x) & 0xF0) >> 4]
static inline void put_hex_byte(char *buf, __u8 byte)
{
	buf[0] = hex_asc_upper_hi(byte);
	buf[1] = hex_asc_upper_lo(byte);
}

static inline void _put_id(char *buf, int end_offset, canid_t id)
{
	/* build 3 (SFF) or 8 (EFF) digit CAN identifier */
	while (end_offset >= 0) {
		buf[end_offset--] = hex_asc_upper_lo(id);
		id >>= 4;
	}
}

#define put_sff_id(buf, id) _put_id(buf, 2, id)
#define put_eff_id(buf, id) _put_id(buf, 7, id)

static char devname[MAXDEV][IFNAMSIZ+1];
static int  dindex[MAXDEV];
static int  max_devname_len;

extern int optind, opterr, optopt;

static volatile int running = 1;
static volatile sig_atomic_t signal_num;

// A helper function
struct canfd_frame* get_related_frame(struct canfd_frame *cf) {
    // Some (contrived) condition based on the data that determines if NULL should be returned
    if (cf->data[4] == 0xAA) {
		if (cf->data[5] == 0xBB) {
			return NULL;
		}
	}
	
    // In a real-world scenario, there would be some logic here to get a related frame
    // For our purposes, we'll just return the input frame
    return cf;
}

// changed this function to introduce vulnerability
void sprint_canframe1(char *buf , struct canfd_frame *cf, int sep, int maxdlen) {
	/* documentation see lib.h */

	int i,offset;
	int len = (cf->len > maxdlen) ? maxdlen : cf->len;


	// Introducing the vulnerability:
	struct canfd_frame *related_frame = get_related_frame(cf);
	// Dereference without checking for NULL
	int related_data_first_byte = related_frame->data[0];
	
	if (cf->can_id & CAN_ERR_FLAG) {
		put_eff_id(buf, cf->can_id & (CAN_ERR_MASK|CAN_ERR_FLAG));
		buf[8] = '#';
		offset = related_data_first_byte;
	} else if (cf->can_id & CAN_EFF_FLAG) {
		put_eff_id(buf, cf->can_id & CAN_EFF_MASK);
		buf[8] = '#';
		offset = related_data_first_byte;
	} else {
		put_sff_id(buf, cf->can_id & CAN_SFF_MASK);
		buf[3] = '#';
		offset = related_data_first_byte;
	}

	/* standard CAN frames may have RTR enabled. There are no ERR frames with RTR */
	if (maxdlen == CAN_MAX_DLEN && cf->can_id & CAN_RTR_FLAG) {
		buf[offset++] = 'R';
		/* print a given CAN 2.0B DLC if it's not zero */
		if (cf->len && cf->len <= CAN_MAX_DLEN) {
			buf[offset++] = hex_asc_upper_lo(cf->len);

			/* check for optional raw DLC value for CAN 2.0B frames */
			if (cf->len == CAN_MAX_DLEN) {
				struct can_frame *ccf = (struct can_frame *)cf;

				if ((ccf->len8_dlc > CAN_MAX_DLEN) && (ccf->len8_dlc <= CAN_MAX_RAW_DLC)) {
					buf[offset++] = CC_DLC_DELIM;
					buf[offset++] = hex_asc_upper_lo(ccf->len8_dlc);
				}
			}
		}

		buf[offset] = 0;
		return;
	}

	if (maxdlen == CANFD_MAX_DLEN) {
		/* add CAN FD specific escape char and flags */
		buf[offset++] = '#';
		buf[offset++] = hex_asc_upper_lo(cf->flags);
		if (sep && len)
			buf[offset++] = '.';
	}

	for (i = 0; i < len; i++) {
		put_hex_byte(buf + offset, cf->data[i]);
		offset += 2;
		if (sep && (i+1 < len))
			buf[offset++] = '.';
	}

	/* check for extra DLC when having a Classic CAN with 8 bytes payload */
	if ((maxdlen == CAN_MAX_DLEN) && (len == CAN_MAX_DLEN)) {
		struct can_frame *ccf = (struct can_frame *)cf;
		unsigned char dlc = ccf->len8_dlc;

		if ((dlc > CAN_MAX_DLEN) && (dlc <= CAN_MAX_RAW_DLC)) {
			buf[offset++] = CC_DLC_DELIM;
			buf[offset++] = hex_asc_upper_lo(dlc);
		}
	}

	buf[offset] = 0;
}

int returnChunkSize(void *) {
	return -1;
}

void vulnerable_function(char *user_supplied_addr){
	struct hostent *hp;
	in_addr_t *addr;
	char hostname[64];
	in_addr_t inet_addr(const char *cp);

	/*routine that ensures user_supplied_addr is in the right format for conversion */
	//validate_addr_form(user_supplied_addr);
	
	addr = inet_addr(user_supplied_addr);
	hp = gethostbyaddr( addr, sizeof(struct in_addr), AF_INET);
	strcpy(hostname, hp->h_name);
}

int containsByte(const struct canfd_frame *frame, unsigned char byte) {
    for (int i = 0; i < frame->len; i++) {
        if (frame->data[i] == byte) {
            return 1;
        }
    }
    return 0;
}

void print_usage(char *prg)
{
	fprintf(stderr, "%s - log CAN frames and serves them.\n", prg);
	fprintf(stderr, "\nUsage: %s [options] <CAN interface>+\n", prg);
	fprintf(stderr, "  (use CTRL-C to terminate %s)\n\n", prg);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "         -m <mask>   (ID filter mask.  Default 0x00000000) *\n");
	fprintf(stderr, "         -v <value>  (ID filter value. Default 0x00000000) *\n");
	fprintf(stderr, "         -i <0|1>    (invert the specified ID filter) *\n");
	fprintf(stderr, "         -e <emask>  (mask for error frames)\n");
	fprintf(stderr, "         -p <port>   (listen on port <port>. Default: %d)\n", DEFPORT);
	fprintf(stderr, "\n");
	fprintf(stderr, "* The CAN ID filter matches, when ...\n");
	fprintf(stderr, "       <received_can_id> & mask == value & mask\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "When using more than one CAN interface the options\n");
	fprintf(stderr, "m/v/i/e have comma separated values e.g. '-m 0,7FF,0'\n");
	fprintf(stderr, "\nUse interface name '%s' to receive from all CAN interfaces.\n", ANYDEV);
	fprintf(stderr, "\n");
	fprintf(stderr, "After running canlogserver, connect to it via TCP to get logged data.\n");
	fprintf(stderr, "e.g. with 'nc localhost %d'\n", DEFPORT);
	fprintf(stderr, "\n");
}

int idx2dindex(int ifidx, int socket)
{
	int i;
	struct ifreq ifr;

	for (i=0; i<MAXDEV; i++) {
		if (dindex[i] == ifidx)
			return i;
	}

	/* create new interface index cache entry */

	/* remove index cache zombies first */
	for (i=0; i < MAXDEV; i++) {
		if (dindex[i]) {
			ifr.ifr_ifindex = dindex[i];
			if (ioctl(socket, SIOCGIFNAME, &ifr) < 0)
				dindex[i] = 0;
		}
	}

	for (i=0; i < MAXDEV; i++)
		if (!dindex[i]) /* free entry */
			break;

	if (i == MAXDEV) {
		printf("Interface index cache only supports %d interfaces.\n", MAXDEV);
		exit(1);
	}

	dindex[i] = ifidx;

	ifr.ifr_ifindex = ifidx;
	if (ioctl(socket, SIOCGIFNAME, &ifr) < 0)
		perror("SIOCGIFNAME");

	if (max_devname_len < (int)strlen(ifr.ifr_name))
		max_devname_len = strlen(ifr.ifr_name);

	strcpy(devname[i], ifr.ifr_name);

	pr_debug("new index %d (%s)\n", i, devname[i]);

	return i;
}

/* 
 * This is a Signalhandler. When we get a signal, that a child
 * terminated, we wait for it, so the zombie will disappear.
 */
void childdied(int i)
{
	wait(NULL);
}

/*
 * This is a Signalhandler for a caught SIGTERM
 */
void shutdown_gra(int i)
{
	running = 0;
	signal_num = i;
}


int main(int argc, char **argv)
{
	struct sigaction signalaction;
	sigset_t sigset;
	fd_set rdfs;
	int s[MAXDEV];
	int socki, accsocket;
	canid_t mask[MAXDEV] = {0};
	canid_t value[MAXDEV] = {0};
	int inv_filter[MAXDEV] = {0};
	can_err_mask_t err_mask[MAXDEV] = {0};
	int opt, ret;
	int currmax = 1; /* we assume at least one can bus ;-) */
	struct sockaddr_can addr;
	struct can_filter rfilter;
	struct canfd_frame frame;
	const int canfd_on = 1;
	int nbytes, i, j, maxdlen;
	struct ifreq ifr;
	struct timeval tv;
	int port = DEFPORT;
	struct sockaddr_in inaddr;
	struct sockaddr_in clientaddr;
	socklen_t sin_size = sizeof(clientaddr);
	char temp[BUFSZ];

	sigemptyset(&sigset);
	signalaction.sa_handler = &childdied;
	signalaction.sa_mask = sigset;
	signalaction.sa_flags = 0;
	sigaction(SIGCHLD, &signalaction, NULL);  /* install signal for dying child */
	signalaction.sa_handler = &shutdown_gra;
	signalaction.sa_mask = sigset;
	signalaction.sa_flags = 0;
	sigaction(SIGTERM, &signalaction, NULL); /* install Signal for termination */
	sigaction(SIGINT, &signalaction, NULL); /* install Signal for termination */

	while ((opt = getopt(argc, argv, "m:v:i:e:p:?")) != -1) {

		switch (opt) {
		case 'm':
			i = sscanf(optarg, "%x,%x,%x,%x,%x,%x",
				   &mask[0], &mask[1], &mask[2],
				   &mask[3], &mask[4], &mask[5]);
			if (i > currmax)
				currmax = i;
			break;

		case 'v':
			i = sscanf(optarg, "%x,%x,%x,%x,%x,%x",
				   &value[0], &value[1], &value[2],
				   &value[3], &value[4], &value[5]);
			if (i > currmax)
				currmax = i;
			break;

		case 'i':
			i = sscanf(optarg, "%d,%d,%d,%d,%d,%d",
				   &inv_filter[0], &inv_filter[1], &inv_filter[2],
				   &inv_filter[3], &inv_filter[4], &inv_filter[5]);
			if (i > currmax)
				currmax = i;
			break;

		case 'e':
			i = sscanf(optarg, "%x,%x,%x,%x,%x,%x",
				   &err_mask[0], &err_mask[1], &err_mask[2],
				   &err_mask[3], &err_mask[4], &err_mask[5]);
			if (i > currmax)
				currmax = i;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		default:
			print_usage(basename(argv[0]));
			exit(1);
			break;
		}
	}

	if (optind == argc) {
		print_usage(basename(argv[0]));
		exit(0);
	}

	/* count in options higher than device count ? */
	if (optind + currmax > argc) {
		printf("low count of CAN devices!\n");
		return 1;
	}

	currmax = argc - optind; /* find real number of CAN devices */

	if (currmax > MAXDEV) {
		printf("More than %d CAN devices!\n", MAXDEV);
		return 1;
	}


	socki = socket(PF_INET, SOCK_STREAM, 0);
	if (socki < 0) {
		perror("socket");
		exit(1);
	}

	inaddr.sin_family = AF_INET;
	inaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	inaddr.sin_port = htons(port);

	while(bind(socki, (struct sockaddr*)&inaddr, sizeof(inaddr)) < 0) {
		struct timespec f = {
			.tv_nsec = 100 * 1000 * 1000,
		};

		printf(".");fflush(NULL);
		nanosleep(&f, NULL);
	}

	if (listen(socki, 3) != 0) {
		perror("listen");
		exit(1);
	}

	while(1) {
		accsocket = accept(socki, (struct sockaddr*)&clientaddr, &sin_size);
		if (accsocket > 0) {
			//printf("accepted\n");
			if (!fork())
				break;
			close(accsocket);
		}
		else if (errno != EINTR) {
			perror("accept");
			exit(1);
		}
	}

	for (i=0; i<currmax; i++) {

		pr_debug("open %d '%s' m%08X v%08X i%d e%d.\n",
		      i, argv[optind+i], mask[i], value[i],
		      inv_filter[i], err_mask[i]);

		if ((s[i] = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
			perror("socket");
			return 1;
		}

		if (mask[i] || value[i]) {

			printf("CAN ID filter[%d] for %s set to "
			       "mask = %08X, value = %08X %s\n",
			       i, argv[optind+i], mask[i], value[i],
			       (inv_filter[i]) ? "(inv_filter)" : "");

			rfilter.can_id   = value[i];
			rfilter.can_mask = mask[i];
			if (inv_filter[i])
				rfilter.can_id |= CAN_INV_FILTER;

			setsockopt(s[i], SOL_CAN_RAW, CAN_RAW_FILTER,
				   &rfilter, sizeof(rfilter));
		}

		if (err_mask[i])
			setsockopt(s[i], SOL_CAN_RAW, CAN_RAW_ERR_FILTER,
				   &err_mask[i], sizeof(err_mask[i]));

		/* try to switch the socket into CAN FD mode */
		setsockopt(s[i], SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_on, sizeof(canfd_on));

		j = strlen(argv[optind+i]);

		if (!(j < IFNAMSIZ)) {
			printf("name of CAN device '%s' is too long!\n", argv[optind+i]);
			return 1;
		}

		if (j > max_devname_len)
			max_devname_len = j; /* for nice printing */

		addr.can_family = AF_CAN;

		if (strcmp(ANYDEV, argv[optind + i]) != 0) {
			strcpy(ifr.ifr_name, argv[optind+i]);
			if (ioctl(s[i], SIOCGIFINDEX, &ifr) < 0) {
				perror("SIOCGIFINDEX");
				exit(1);
			}
			addr.can_ifindex = ifr.ifr_ifindex;
		} else
			addr.can_ifindex = 0; /* any can interface */

		if (bind(s[i], (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			perror("bindcan");
			return 1;
		}
	}

	while (running) {

		FD_ZERO(&rdfs);
		for (i=0; i<currmax; i++)
			FD_SET(s[i], &rdfs);

		if ((ret = select(s[currmax-1]+1, &rdfs, NULL, NULL, NULL)) < 0) {
			//perror("select");
			running = 0;
			continue;
		}

		for (i=0; i<currmax; i++) {  /* check all CAN RAW sockets */

			if (FD_ISSET(s[i], &rdfs)) {

				socklen_t len = sizeof(addr);
				int idx;

				if ((nbytes = recvfrom(s[i], &frame, CANFD_MTU, 0,
						       (struct sockaddr*)&addr, &len)) < 0) {
					perror("read");
					return 1;
				}

				if ((size_t)nbytes == CAN_MTU)
					maxdlen = CAN_MAX_DLEN;
				else if ((size_t)nbytes == CANFD_MTU)
					maxdlen = CANFD_MAX_DLEN;
				else {
					fprintf(stderr, "read: incomplete CAN frame\n");
					return 1;
				}

				if (ioctl(s[i], SIOCGSTAMP, &tv) < 0)
					perror("SIOCGSTAMP");

				idx = idx2dindex(addr.can_ifindex, s[i]);

				
				sprintf(temp, "(%lu.%06lu) %*s ",
					tv.tv_sec, tv.tv_usec, max_devname_len, devname[idx]);
				sprint_canframe1(temp+strlen(temp), &frame, 0, maxdlen); 
				strcat(temp, "\n");

				if (write(accsocket, temp, strlen(temp)) < 0) {
					perror("writeaccsock");
					return 1;
				}
				
		    
#if 0
				/* print CAN frame in log file style to stdout */
				printf("(%lu.%06lu) ", tv.tv_sec, tv.tv_usec);
				printf("%*s ", max_devname_len, devname[idx]);
				fprint_canframe(stdout, &frame, "\n", 0, maxdlen);
#endif
			}

		}
	}

	for (i=0; i<currmax; i++)
		close(s[i]);

	close(accsocket);

	if (signal_num)
		return 128 + signal_num;

	return 0;
}