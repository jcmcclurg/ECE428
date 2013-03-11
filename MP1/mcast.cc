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

/* Failure detection */
pthread_t heartbeat_thread;

void *heartbeat_thread_main(void *arg) {
  timespec req;
  req.tv_sec = 0;
  req.tv_nsec = HEARTBEAT_SECONDS * 1000000L;
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

void multicast_init(void) {
  unicast_init();
  globalState = new GlobalState(my_id, mcast_members, mcast_num_members);
}

void multicast_deliver(int id, Message* m){
  deliver(my_id, (char*) m->getMessage().c_str());
}

/* Basic multicast implementation */
void multicast(const char *message) {
  //! Increment the timestamp before you do anything else.
  //timestamp_increment();

  vector<pair<int, int> > acknowledgements;
  map<int, ExternalNodeState*> externalStates = globalState->externalStates;
  for (
      map<int, ExternalNodeState*>::iterator it = externalStates.begin();
      it != externalStates.end();
      it++) {

    acknowledgements.push_back(make_pair<int, int>(
      it->first, it->second->getLatestDeliveredSequenceNumber()
    ));
  }

  //! Wrap the message in a Message object.
  NodeState& state = globalState->state;
  Message* m = new Message(
    state.getId(),
    state.getSequenceNumber(),
    state.getTimestamp(),
    MESSAGE,
    string(message),
    acknowledgements
  );

  //! Deliver to yourself first, so that if you fail before sending it to everyone, the message can still 
  // get re-transmitted by someone else without violating the R-multicast properties.
  multicast_deliver(my_id, m);

  //! Add to the sent messages list so you can grab this message again in case a retransmission is needed.
  //add_to_sent_list(m);

  pthread_mutex_lock(&member_lock);
  for (int i = 0; i < mcast_num_members; i++) {
      usend(mcast_members[i], message, strlen(message)+1);
  }
  pthread_mutex_unlock(&member_lock);

  //! Increment your own sent message sequence number.
  //sequence_number++;
}

void mcast_join(int member) {
  printf("%d friggin joined.", member);
}

void receive(int source, const char *message, int len) {
  assert(message[len-1] == 0);

  if(source != my_id){
    deliver(source, message);
  }
}