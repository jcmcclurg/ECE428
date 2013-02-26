#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <map>

#include "mp1.h"

/* Defines */

#define PIGGYBACK_DELIMITER '\x17'

#define HEARTBEAT_DELAY 10
#define HEARTBEAT_MESSAGE "xoxo"

/* General */

// Returns a newly allocated string. Remember to delete it.
char* piggy_back(const char* message, const char* aux) {
    // Extra two characters for delimiter and null
    int message_len = strlen(message);
    int aux_len = strlen(aux);

    char* new_message = (char*) calloc(strlen(message) + strlen(aux) + 2, 1);
    strcpy(new_message, message);
    new_message[message_len] = PIGGYBACK_DELIMITER;
    strcpy(new_message + message_len + 1, aux);
    new_message[message_len + 1 + aux_len] = 0;

    return new_message;
}

std::map<int, int> id_map;

void id_map_init(void) {
    for (int i = 0; i < mcast_num_members; ++i) {
        id_map.insert(std::make_pair(mcast_members[i], i));
    }
}

/* Timestamp */

int *timestamp;

void timestamp_init(void) {
    // Extra slot for easy stringification.
    timestamp = (int*) calloc(mcast_num_members + 1, sizeof(int));
    memset(timestamp, 0, mcast_num_members + 1);
}

void timestamp_increment() {
    timestamp[id_map[my_id]]++;
}

// Ultra hacky int array stringification.
const char* timestamp_stringify() {
    return (const char*) timestamp;
}

void timestamp_destroy() {
    free(timestamp);
}

// Failure detection

bool* failed_processes;
pthread_t heartbeat_thread;

void *heartbeat_thread_main(void *arg) {
    timespec req;
    req.tv_sec = 0;
    req.tv_nsec = HEARTBEAT_DELAY * 1000000L;
    while(1) {
        nanosleep(&req, NULL);
    }
}

void heartbeat_init(void) {
    if (pthread_create(&heartbeat_thread, NULL, &heartbeat_thread_main, NULL) != 0) {
        fprintf(stderr, "Error creating heartbeat thread!\n");
        exit(1);
    }
}

void heartbeat_destroy(void) {

}

void multicast_init(void) {
    unicast_init();
    id_map_init();
    timestamp_init();
}

/* Basic multicast implementation */
void multicast(const char *message) {
    timestamp_increment();
    char *new_message = piggy_back(message, timestamp_stringify());

    int i;

    pthread_mutex_lock(&member_lock);
    for (i = 0; i < mcast_num_members; i++) {
        usend(mcast_members[i], message, strlen(message)+1);
    }
    pthread_mutex_unlock(&member_lock);

    free(new_message);
}

void receive(int source, const char *message, int len) {
    assert(message[len-1] == 0);
    deliver(source, message);
}

void mcast_join(int member) {
    printf("%d friggin joined.", member);
}
