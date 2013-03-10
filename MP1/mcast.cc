#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <map>
#include <vector>
#include <algorithm>
#include <ctime>
#include <sstream>

#include "mp1.h"
#include "delivery/delivery_ack.h"
#include "message/message.h"
#include "timestamp/timestamp.h"

using namespace std;

/* Defines */

#define HEARTBEAT_DELAY 10
#define HEARTBEAT_MESSAGE "xoxo"

#define TIMEOUT_SECONDS 60

using namespace std;

/* State variables */

//! Mapping from ID to node state
map<int, NodeState*> global_state;

//! The following pointer lists are used to update all the data structures when group membership changes.
vector<Timestamp*> timestamp_list;
vector<DeliveryAcks*> deliveryAck_list;

/**
 * Initializes the global state map.
 */
void state_init(void) {
  for (int i = 0; i < mcast_num_members; ++i) {
    Timestamp* t= new Timestamp(mcast_members[i]);
    timestamp_list.push_back(t);
    global_state[mcast_members[i]] = new NodeState(t);
    global_state[mcast_members[i]]->resetTimeout();
  }
  DeliveryAcks* d = new DeliveryAcks(my_id);
  deliveryAck_list.push_back(d);
}

/**
 * Increment the vector timestamp.
 */
 void timestamp_increment() {
   global_state[my_id]->timestamp->step();
 }

/**
 * Merges external timestamp with the existing timestamp. Assumes other_timestamp is same size and is ordered the same as local timestamp.
 * @param [in] other_timestamp
 */
 void timestamp_merge(Timestamp* other_timestamp){
   global_state[my_id]->timestamp->update(other_timestamp);
 }

/**
 * Updates global state to include a new multicast member.
 * @param [in] member
 */
 void mcast_join(int member) {
    // Make certain this really is a new member. If not, don't do anything.
   // Update global_state, timestamp_list, and deliveryAck_list.
   // FIXME: Implement.
 }

/**
 * Updates global state to exclude an existing multicast member.
 * @param [in] member
 */
 void mcast_kick(int member) {
    // Make certain this really is a person in the group. If not, don't do anything.
   // Update global_state, timestamp_list, and deliveryAck_list.
   // FIXME: Implement.
 }

/**
 * De-allocates the timestamp memory.
 */
 void state_teardown() {
   // Do opposite of state_init()
   // FIXME: Implement.
 }

/* General */


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
  state_init();
}

/* Basic multicast implementation */
void multicast(const char *message) {
  timestamp_increment();

  char* combined_message = NULL;
  int total_bytes;
  int* timestamp;

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

  deliver(source, message);
}

