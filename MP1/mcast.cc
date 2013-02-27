#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <map>
#include <vector>

#include "mp1.h"

/* Defines */

#define MESSAGE_HEADER '\x1f'
#define TIMESTAMP_HEADER '\x1d'

#define HEARTBEAT_DELAY 10
#define HEARTBEAT_MESSAGE "xoxo"

/* General */

// Flattens a bunch of strings and a timestamp into a serialized string.
void flatten(
        std::vector<const char*>& messages,
        int* timestamp,
        char*& combined_message,
        int& total_bytes) {

    int *message_bytes = (int*) calloc(messages.size(), sizeof(int)); 
    int timestamp_bytes = mcast_num_members * sizeof(int);

    total_bytes = 0;
    for (int i = 0; i < messages.size(); ++i) {
        int bytes = strlen(messages[i]) + 1;
        message_bytes[i] = bytes;
        total_bytes += bytes;
    }
    // The extra bytes are for the control headers. 
    total_bytes += messages.size() + 1 + timestamp_bytes;

    combined_message = (char*) calloc(total_bytes, 1);

    int offset = 0;
    for (int i = 0; i < messages.size(); ++i) {
        combined_message[offset] = MESSAGE_HEADER;
        offset++;
        memcpy(combined_message + offset, messages[i], message_bytes[i]);
        offset += message_bytes[i];
    }

    // Add timestamp
    combined_message[offset] = TIMESTAMP_HEADER;
    offset++;
    memcpy(combined_message + offset, timestamp, timestamp_bytes);

    free(message_bytes);
}

void unflatten(
        const char* combined_message,
        int total_bytes,
        std::vector<const char*>& messages,
        int*& timestamp) {

    int offset = 1;
    while (offset < total_bytes) {
        if (combined_message[offset - 1] == MESSAGE_HEADER) {
            int bytes = strlen(combined_message + offset) + 1;
            messages.push_back(combined_message + offset);
            offset += bytes + 1;
        } else if (combined_message[offset - 1] == TIMESTAMP_HEADER) {
            timestamp = (int*) (combined_message + offset);
            return;
        } else {
            printf("Error! Poorly formatted message!\n");
            return;
        }
    }
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
    timestamp = (int*) calloc(mcast_num_members, sizeof(int));
    memset(timestamp, 1, mcast_num_members);
}

void timestamp_increment() {
    timestamp[id_map[my_id]]++;
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
    
    char* combined_message = NULL;
    int total_bytes;

    std::vector<const char*> messages;
    messages.push_back("abc");
    messages.push_back("omg");

    flatten(messages, timestamp, combined_message, total_bytes);

    pthread_mutex_lock(&member_lock);
    for (int i = 0; i < mcast_num_members; i++) {
        usend(mcast_members[i], combined_message, total_bytes);
    }
    pthread_mutex_unlock(&member_lock);

    free(combined_message);
}

void receive(int source, const char *message, int len) {
    //assert(message[len-1] == 0);

    std::vector<const char*> messages;
    int* other_timestamp;    
    unflatten(message, len, messages, other_timestamp);

    printf("other timestamp: %d\n", other_timestamp[id_map[my_id]]);
    printf("last timestamp: %d\n", other_timestamp[mcast_num_members - 1]);

    deliver(source, message);
}

void mcast_join(int member) {
    printf("%d friggin joined.", member);
}
