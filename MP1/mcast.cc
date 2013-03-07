#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <map>
#include <vector>
#include <algorithm>

#include "mp1.h"

/* Defines */

#define MESSAGE_HEADER '\x1f'
#define TIMESTAMP_HEADER '\x1d'

#define HEARTBEAT_DELAY 10
#define HEARTBEAT_MESSAGE "xoxo"

//TODO: Probably you can think of better data structures, but this gets the job done I guess.
/* State variables */

struct NodeState {
	int id;
	int* timestamp;
};

struct IDMap{
	int size;
	NodeState* states;
};

//! ID map contains the local perception of the global state.
IDMap id_map;

/**
 * Initializes the global state map.
 */
void state_init(void) {
	id_map.size = mcast_num_members;
	id_map.states = (NodeState*) malloc(sizeof(NodeState)*id_map.size);

	for (int i = 0; i < id_map.size; ++i) {
		id_map.states[i].id = mcast_members[i];
		id_map.states[i].timestamp = (int*) calloc(id_map.size,sizeof(int));
	}
}

/**
 * Increment the vector timestamp.
 */
void timestamp_increment() {
    id_map.states[my_id].timestamp[my_id]++;
}

/**
 * Merges external timestamp with the existing timestamp. Assumes other_timestamp is same size and is ordered the same as local timestamp.
 * @param [in] other_timestamp
 */
void timestamp_merge(int* other_timestamp){
	for(int i = 0; i < id_map.size; ++i){
		if(my_id != i){
			id_map.states[my_id].timestamp[i] = max(other_timestamp[i], id_map.states[my_id].timestamp[i]);
		}
	}
}

/**
 * Updates global state to include a new multicast member.
 * @param [in] member
 */
void mcast_join(int member) {
	// Make certain this really is a new member. If not, don't do anything.
	for (int i = 0; i < id_map.size; ++i) {
		if(id_map.states[i].id == member){
			return;
		}
	}

	// Allocate a new ID list.
 	id_map.size++;
	NodeState* temp = (NodeState*) malloc(sizeof(NodeState)*(id_map.size));

	// Copy old pointers to new list.
	for (int i = 0; i < id_map.size-1; ++i) {
		temp[i].id = id_map.states[i].id;
		temp[i].timestamp = id_map.states[i].timestamp;
	}

	// Add the new member to the end.
	temp[id_map.size-1].id = member;
	temp[id_map.size-1].timestamp = (int*) calloc(id_map.size,sizeof(int))

	// Free old list and update.
	free(id_map.states);
	id_map.states = temp;
}

/**
 * Updates global state to exclude an existing multicast member.
 * @param [in] member
 */
void mcast_kick(int member) {
	// Make certain this really is a person in the group. If not, don't do anything.
	int num = -1;
	for (int i = 0; i < id_map.size; ++i) {
		if(id_map.states[i].id == member){
			num = i;
			break;
		}
	}
	if(num == -1){
		return;
	}
	// Allocate a new ID list.
	NodeState* temp = (NodeState*) malloc(sizeof(NodeState)*(id_map.size-1));

	// Copy old pointers to new list.
	for (int i = 0; i < id_map.size; ++i) {
		if(i < num){
			temp[i].id = id_map.states[i].id;
			temp[i].timestamp = id_map.states[i].timestamp;
		}
		else if(i > num){
			temp[i-1].id = id_map.states[i].id;
			temp[i-1].timestamp = id_map.states[i].timestamp;
		}
	}

	// Free the old member.
	free(temp[num].timestamp)

	// Free old list and update.
	free(id_map.states);
 	id_map.size--;
	id_map.states = temp;
}

/**
 * De-allocates the timestamp memory.
 */
void state_teardown() {
	for(int i = 0; i < id_map.size; ++i){
		free(id_map.states[i].timestamp);
	}
	free(id_map.states);
}

/* General */

/**
 * Encodes timestamp, message, and metadata into serialized packet for transmission.
 * @param [in]  messages
 * @param [in]  timestamp
 * @param [out] combined_message
 * @param [out] total_bytes
 */
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

/**
 * Decodes timestamp, message, and metadata from packets encoded with flatten.
 * @param [in]  combined_message
 * @param [in]  total_bytes
 * @param [out] messages
 * @param [out] timestamp
 */
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

/* Failure detection */

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

