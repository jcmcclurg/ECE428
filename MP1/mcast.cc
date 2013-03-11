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

#include "message/message.h"
#include "state/state.h"
#include "timestamp/timestamp.h"
#include "mp1.h"

/* Defines */
#define HEARTBEAT_SECONDS 10
#define TIMEOUT_SECONDS 60

//! The global state information
GlobalState* globalState;

void multicast_init(void) {
  unicast_init();
  globalState = new GlobalState(my_id, mcast_members, mcast_num_members);
}

void multicast_deliver(int id, Message* m){
  deliver(my_id, (char*) m->getMessage().c_str());
}

void discard(Message* m){
  // Remove from message store
  // Free up memory
}

/**
 * Reliable multicast implementation.
 */
void multicast(const char *message) {
  NodeState& state = globalState->state;
  map<int, ExternalNodeState*> externalStates = globalState->externalStates;

  //! Increment the timestamp before you do anything else.
  state.timestampIncrement();

  vector<pair<int, int> > acknowledgements;
  for (
      map<int, ExternalNodeState*>::iterator it = externalStates.begin();
      it != externalStates.end();
      it++) {

    acknowledgements.push_back(make_pair<int, int>(
      it->first, it->second->getLatestDeliveredSequenceNumber()
    ));
  }

  //! Wrap the message in a Message object.
  Message* m = new Message(
    state.getId(),
    state.getSequenceNumber(),
    state.getTimestamp(),
    MESSAGE,
    string(message),
    acknowledgements
  );

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
  state.sequenceNumberIncrement();
}

void mcast_join(int member) {
  printf("%d friggin joined.", member);
}

void receive(int source, const char *message, int len) {
  assert(message[len-1] == 0);
  assert(source != my_id);

  NodeState& state = globalState->state;

  // Increment the timestamp before you do anything else.
  state.timestampIncrement();

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
  deliver(source, message);
}