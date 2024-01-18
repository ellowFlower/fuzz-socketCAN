#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <netinet/in.h>
#include <netdb.h>

#define START_INDEX 4
#define END_INDEX 19
#define NUM_NODES (END_INDEX - START_INDEX + 1)
#define SONG_COUNT 5

char *songs[SONG_COUNT] = {
    "Song 1: Shape of You",
    "Song 2: Rolling in the Deep",
    "Song 3: Uptown Funk",
    "Song 4: Let It Be",
    "Song 5: Sweet Child O' Mine"
};

struct VehicleState {
    int front_light_state;  // 0: off, 1: on
    int heating;
    int aircon;
    int privileged;
    int speed;
    int software_type;
};
struct VehicleState vehicle_state;

void state(struct canfd_frame frame) {
    for (int i = 0; i < frame.len; i++) {
        if (frame.data[i] != 0x00) {
            return;
        }
    }
    printf("Vehicle State: \n");
    printf("front light: %i \n", vehicle_state.front_light_state);
    printf("heating: %i \n", vehicle_state.heating);
    printf("aircon: %i \n", vehicle_state.aircon);
    printf("privileged: %i \n", vehicle_state.privileged);
    printf("speed: %i \n", vehicle_state.speed);
    printf("\n");
}

void frontLight(struct canfd_frame frame) {
    if (frame.data[0] == 0x00 && frame.data[1] == 0x01) {
        printf("front light on\n");
        vehicle_state.front_light_state = 1;
    } else if (frame.data[0] == 0x00 && frame.data[1] == 0x02) {
        printf("front light off\n");
        vehicle_state.front_light_state = 0;
    }
}

void heating(struct canfd_frame frame) {
    if (frame.data[0] == 0x00 && frame.data[1] == 0x03) {
        printf("heating on\n");
        vehicle_state.heating = 1;
    } else if (frame.data[0] == 0x00 && frame.data[1] == 0x04) {
        printf("heating off\n");
        vehicle_state.heating = 0;
    }
}

void aircon(struct canfd_frame frame) {
    if (frame.data[0] == 0x00 && frame.data[1] == 0x05) {
        printf("aircon on\n");
        vehicle_state.aircon = 1;
    } else if (frame.data[0] == 0x00 && frame.data[1] == 0x06) {
        printf("aircon off\n");
        vehicle_state.aircon = 0;
    }
}

void privileged_pw_check(struct canfd_frame frame) {
    if (frame.data[3] == 0x55) {
        if (frame.data[4] == 0xFF) {
            vehicle_state.privileged = 1;
            printf("privilege mode on \n");
        }
    }
}

void access_engine(struct canfd_frame frame) {
    int distances[NUM_NODES];
    int visited[NUM_NODES] = {0};

    for (int i = 0; i < NUM_NODES; i++) {
        distances[i] = INT_MAX;
    }
    distances[0] = 0;

    for (int i = 0; i < NUM_NODES - 1; i++) {
        int minDist = INT_MAX;
        int u;

        for (int j = 0; j < NUM_NODES; j++) {
            if (visited[j] == 0 && distances[j] <= minDist) {
                minDist = distances[j];
                u = j;
            }
        }

        visited[u] = 1; 

        for (int k = u+1; k < NUM_NODES; k++) {
            if (!visited[k] && distances[u] != INT_MAX && distances[u] + frame.data[START_INDEX + u] < distances[k]) {
                distances[k] = distances[u] + frame.data[START_INDEX + u];
            }
        }
    }

    printf("engine state is %d \n", distances[NUM_NODES - 1]);

    // CRASH
    // out-of-bounds write
    // cansend can0 550##00001
    // cansend can0 550##00005
    // cansend can0 550##000003344010ed45FF3380901025566884477
    if (vehicle_state.front_light_state == 1) {
        if (vehicle_state.aircon == 1) {
            char destBuf[10];
            char srcBuf[10];
            memcpy(destBuf, srcBuf, (distances[NUM_NODES - 1]-2));
        }
    }
}

void access_maintenance(struct canfd_frame frame) {
    if (frame.data[3] == 0x44) {
        access_engine(frame);
    }
}

int calculate() {
    return -1;
}

void playSong(int choice) {
    if (choice > 0 && choice <= SONG_COUNT) {
        printf("Now playing %s\n\n", songs[choice - 1]);
    } else {
        printf("Invalid choice!\n\n");
    }
}

void access_mediacenter(struct canfd_frame frame) {
    char *product1[3] = {"A", "B", "C"};
    char *product2[3] = {"X", "Y", "Z"};
    char *product3[3] = {"W", "E", "R"};

    if (frame.data[3] == 0x44) {
        printf("product1: %s %s %s\n", product1[0], product1[1], product1[2]);
        return;
    }
    if (frame.data[3] == 0x66) {
        printf("product2: %s %s %s\n", product2[0], product2[1], product2[2]);
        return;
    }
    if (frame.data[3] == 0xEC) {
        printf("product3: %s %s %s\n", product3[0], product3[1], product3[2]);
        return;
    }

    if (frame.data[3] <= 0x33) {
        // play music
        int sum = 0;
        for (int i = 4; i <= 19; i++) {
            sum += frame.data[i];
        }
        // CRASH
        playSong((sum % 7) + 1); // it should be modulo 5 to only acces valid variables in song struct
        return;
    }
    
    // CRASH
    // change product 
    // out-of-bounds write
    // cansend can0 550##000000155FF
    // cansend can0 550##0000055EE
    if (frame.data[3] >= 0xEC) {
        printf("change product 2 \n");
        char *newProduct[3] = {"N", "E", "W"};
        memcpy(product2, newProduct, (calculate()-1));
    }
}

void privileged_actions(struct canfd_frame frame) {
    // nested function calls depending on state or message
    if (frame.data[2] == 0x33) {
        access_maintenance(frame);
    }

    if (frame.data[2] == 0x55) {
        access_mediacenter(frame);
    }
}

void calculate_speed(struct canfd_frame frame) {
    // frame.data[3] and frame.data[4] are rpm from wheel
    int rpm = frame.data[3] | (frame.data[4] << 8);
    double linear_speed_m_per_sec = (rpm / 60.0) * 2.0; 
    double linear_speed_km_per_h = linear_speed_m_per_sec * 3.6;
    vehicle_state.speed = linear_speed_km_per_h;
    printf("speed is %f \n", linear_speed_km_per_h);
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
	strcpy(hostname, hp->h_name); // CRASH
}

int main() {
    int s; // socket
    struct sockaddr_can addr;
    struct canfd_frame frame;
    struct ifreq ifr;
    int enable_fd = 1;

    const char *ifname = "can0";

    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Error while opening socket");
        return -1;
    }

    if (setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable_fd, sizeof(enable_fd)) < 0) {
        perror("Socket option setting error");
        close(s);
        return -1;
    }

    strcpy(ifr.ifr_name, ifname);
    ioctl(s, SIOCGIFINDEX, &ifr);

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Error in socket bind");
        return -2;
    }

    struct can_filter rfilter[1];
	rfilter[0].can_id   = 0x550;
	rfilter[0].can_mask = 0xFF0;
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

    while (1) {
        ssize_t nbytes = read(s, &frame, sizeof(struct canfd_frame));

        if (nbytes < 0) {
            perror("Error reading from socket");
            continue;
        }

        state(frame);

        if (vehicle_state.privileged) {
            privileged_actions(frame);
        }

        if (frame.data[0] == 0x00 && frame.data[1] == 0x00) {
            if (frame.data[2] == 0x01) {
                privileged_pw_check(frame);
            }
            if (frame.data[2] == 0x45) {
                calculate_speed(frame);
            }
        } else {
            frontLight(frame);
            heating(frame);
            aircon(frame);
        }

        if (vehicle_state.aircon && vehicle_state.front_light_state && vehicle_state.heating) {
            abort(); // CRASH
        }
    }

    close(s);

    return 0;
}
