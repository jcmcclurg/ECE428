#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <map>
#include <iostream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <sstream>

using namespace std;

#include "../state/node_state.hpp"
#include "mp1.h"

/* Defines */

#define MESSAGE_HEADER '\x1f'
#define TIMESTAMP_HEADER '\x1d'

#define HEARTBEAT_DELAY 10
#define HEARTBEAT_MESSAGE "xoxo"

#define TIMEOUT_SECONDS 60

//! The global state information
GlobalState globalState(my_id);

/**
 * Increment the vector timestamp.
 */
void timestamp_increment() {
  global_timestamp->step();
}

/**
 * Merges external timestamp with the existing timestamp. Assumes other_timestamp is same size and is ordered the same as local timestamp.
 * @param [in] other_timestamp
 */
void timestamp_merge(Timestamp* other_timestamp){
  global_timestamp->update(other_timestamp);
}

/**
 * De-allocates the timestamp memory. Does the opposite of state_init().
 */
void state_teardown() {
  for (map<int,NodeState*>::iterator it=global_state.begin(); it != global_state.end(); ++it){
    delete global_state[it->first];
  }

  for (vector<Timestamp*>::iterator it=timestamp_list.begin(); it != timestamp_list.end(); ++it){
    delete *it;
  }

  for (vector<DeliveryAcks*>::iterator it=deliveryAck_list.begin(); it != deliveryAck_list.end(); ++it){
    delete *it;
  }
}

/* Failure detection */
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

void add_to_message_store(Message* m){
  state->messageStore[m->sequenceN]
  global_state[my_id]->sentMessages[m->sequence_number] = m;
}

void multicast_init(void) {
  unicast_init();
  state_init();
}

void multicast_deliver(int id, Message* m){
  deliver(my_id, (char*) m->getMessage().c_str());
}

void multicast(Message* m) {
  multicast();
}

/* Basic multicast implementation */
void multicast(const char *message) {
  //! Increment the timestamp before you do anything else.
  timestamp_increment();

  //! Wrap the message in a Message object.
  Message* m = new Message(MESSAGE,
    sequence_number,
    my_id,
    global_state[my_id]->deliveryAckList,
    &failed_nodes,
    global_timestamp,
    string(message));

  //! Deliver to yourself first, so that if you fail before sending it to everyone, the message can still 
  // get re-transmitted by someone else without violating the R-multicast properties.
  multicast_deliver(my_id, m);

  //! Add to the sent messages list so you can grab this message again in case a retransmission is needed.
  add_to_sent_list(m);

  pthread_mutex_lock(&member_lock);
  for (int i = 0; i < mcast_num_members; i++) {
      usend(mcast_members[i], message, strlen(message)+1);
  }
  pthread_mutex_unlock(&member_lock);

  //! Increment your own sent message sequence number.
  sequence_number++;
}

void receive(int source, const char *message, int len) {
  assert(message[len-1] == 0);

  if(source != my_id){
    deliver(source, message);
  }
}