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

void discard(Message* m){
  // Remove from message store
  // Free up memory
}

/**
 * Reliable multicast implementation.
 */
void multicast(const char *message) {
  pthread_mutex_lock(&member_lock);

  // Increment the timestamp before you do anything else.
  timestamp_increment();

  // Wrap the message in a Message object.
  Message* m = new Message(MESSAGE,
    sequence_number,
    my_id,
    global_state[my_id]->deliveryAckList,
    &failed_nodes,
    global_timestamp,
    string(message));

  // Deliver to yourself first, so that if you fail before sending it to everyone, the message can still 
  // get re-transmitted by someone else without violating the R-multicast properties.
  multicast_deliver(my_id, m);

  // Add to the sent messages list so you can grab this message again in case a retransmission is needed.
  add_to_message_store(m);

  // Unicast all the messages.
  for (map<int,NodeState*>::iterator it=global_state.begin(); it != global_state.end(); ++it){
    // There's no need to send it to ourselves since we've already delivered the message.
    if(it->first != my_id){
      usend(it->first, m->getEncodedMessage(), m->getEncodedMessageLength());
    }
  }

  // Increment your own sent message sequence number.
  sequence_number++;

  pthread_mutex_unlock(&member_lock);
}

void receive(int source, const char *message, int len) {
  assert(message[len-1] == 0);
  assert(source != my_id);

  // Increment the timestamp before you do anything else.
  timestamp_increment();

  // De-serialize the message into a new Message object. 
  Message* m = new Message(string(message));

  // Update our state based on receive information from node m->id.
  update_global_timestamp(m->timestamp);
  add_to_message_store(m);
  update_message_store(m->id,m->deliveryAckList);
  update_failed_node_list(m->failed_nodes);
  reset_node_timeouts();

  if(m->type == MessageType::RETRANSMISSION){
    int from_id;
    int to_id;
    int seq_num;
    decode_retransmission_message(m->getMessage(),&from_id,&to_id,&seq_num);
    Message* m = get_message_from_store(from_id,seq_num);
    unicast(to_id,m->getEncodedMessage(), m->getEncodedMessageLength());
  }
  else if(m->type == MessageType::MESSAGE){
    // Have you already delivered this message?
    if(m->sequenceNumber <= global_delivery_list(m->id) ){
      discard(m);
    }

    // Have you already gotten this message?
    else if(!in_holdback_queue(m)){
      add_to_holdback_queue(m);

      // Try to deliver some of the messages
      bool delivered=true;
      while(delivered){
        delivered =false;
        vector<Message*> deliverables;
        for(each message a in holdback queue){
          // Is this the message you are expecting?
          if(a->sequenceNumber == global_delivery_list(a->id) + 1){
            deliverables.push_end(a);
            delivered = true;
          }
        }

        // Sort the messages according to the partial order determined by their timestamps, such that
        // the causality relation is maintained.
        partial_order_message_list = sort_messages(&deliverables);

        // Recursively delivers messages along the tree. The behavior is as follows:
        // 1. Start with the end node.
        // 2. For each node
        //    a. Iterate through the predecessor nodes if possible.
        //    b. Deliver the node's message.
        deliver_along_partial_order_tree(partial_order_message_list);
      }

      // Find all the sequence numbers that we are missing.
      map<int,int> seqNumbers = findMissingSequenceNumbers();

      // Ask for re-transmissions from everyone known to have delivered the messages we need.
      // The reason we ask everyone is that the original sender may have failed.
      for(each id, seqnumber in seqNumbers){
        vector<int> nodes = find_nodes_that_delivered_seqNumber(seqnumber);
        assert(nodes not empty);

        Message* r = new Message(retransmission request for message id,seqnumber);

        for(each to_id in nodes){
          unicast(to_id,m->getEncodedMessage(), m->getEncodedMessageLength());
        }
      }
    }
    else{
      discard(m);
    }
  }
  else{ // if(m->type == HEARTBEAT)
    discard(m);
  }

  timestamp_update();
}
