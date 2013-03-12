#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <map>
#include <iostream>
#include <set>
#include <algorithm>
#include <ctime>
#include <stdlib.h>
#include <sstream>

using namespace std;

#include "heartbeat/heartbeat.h"
#include "message/message.h"
#include "state/state.h"
#include "timestamp/timestamp.h"
#include "mp1.h"

/* Defines */
#define HEARTBEAT_MS 10 * 1000L
#define TIMEOUT_MS 60 * 1000L

//! The global state information
GlobalState* globalState;
Heartbeat* heartbeat;
char encodeBuffer[100];

//! Forward declaration for our multicast function.
void multicast(const char *message, MessageType type);

static void heartbeat_failure(int sig, siginfo_t *si, void *uc) {
  int failedId = si->si_value.sival_int;
  globalState->state.getFailedNodes().insert(failedId);
}

static void heartbeat_send() {
  multicast("xoxo", HEARTBEAT);
}

void unicast(int to, Message& m){
  int sz = m.getEncodedMessage(encodeBuffer);
  usend(to, encodeBuffer, sz);
}

/**
 * IMPORTANT: Assumes that the group membership will not grow during runtime.
 */
void multicast_init(void) {
  unicast_init();
}

void multicast_deliver(Message& m){
  if (m.getSenderId() == my_id) {
    globalState->state.sequenceNumberIncrement();
  } else {
    globalState->externalStates[m.getSenderId()]->latestDeliveredSequenceNumberIncrement();
  }
  deliver(m.getSenderId(), m.getMessage().c_str());
}

static void populateAcknowledgements(
      map<int, int>& acknowledgements, 
      map<int, ExternalNodeState*>& externalStates) {

  for (
      map<int, ExternalNodeState*>::iterator it = externalStates.begin();
      it != externalStates.end();
      it++) {

    acknowledgements[it->first] = it->second->getLatestDeliveredSequenceNumber();
  }
}

void initIfNecessary(){
  if(globalState == NULL){
    globalState = new GlobalState(my_id, mcast_members, mcast_num_members);
    heartbeat = new Heartbeat(
      mcast_members, 
      mcast_num_members, 
      TIMEOUT_MS, 
      HEARTBEAT_MS, 
      heartbeat_failure,
      heartbeat_send
    );
    heartbeat->arm();
  }
}
/**
 * Reliable multicast implementation.
 */
void multicast(const char *message, MessageType type) {
  initIfNecessary();

  NodeState& state = globalState->state;
  map<int, ExternalNodeState*> externalStates = globalState->externalStates;

  // Increment the timestamp before you do anything else.
  state.getTimestamp().step();

  map<int, int> acknowledgements;
  populateAcknowledgements(acknowledgements, externalStates);

  // Wrap the message in a Message object.
  Message* m = new Message(
    state.getId(),
    state.getSequenceNumber(),
    state.getTimestamp(),
    type,
    string(message),
    acknowledgements,
    state.getFailedNodes()
  );

  // Deliver to yourself first, so that if you fail before sending it to everyone, the message can still 
  // get re-transmitted by someone else without violating the R-multicast properties.
  if (type != HEARTBEAT) {
    multicast_deliver(*m);

    // Add to the sent messages list so you can grab this message again in case a retransmission is needed.
    state.storeMessage(m);
  }

  // Unicast all the messages.
  for (
      map<int, ExternalNodeState*>::iterator it = externalStates.begin();
      it != externalStates.end();
      it++) {

    // There's no need to send it to ourselves since we've already delivered the message.
    if(it->first != my_id) {
      unicast(it->first, *m);
    }
  }

  // Increment your own sent message sequence number.
  // state.sequenceNumberIncrement();
}

void multicast(const char *message) {
  multicast(message, MESSAGE);
}

void mcast_join(int member) {
  #ifdef DEBUG
  cout << "Member " << member << " joined" << endl;
  #endif
}

void discard(Message* m) {
  // Free up some memory or something?
  delete m;
}

void receive(int source, const char *message, int len) {
  initIfNecessary();
  assert(source != my_id);

  NodeState& state = globalState->state;
  map<int, ExternalNodeState*> externalStates = globalState->externalStates;

  // Increment the timestamp before you do anything else.
  state.getTimestamp().step();

  // De-serialize the message into a new Message object. 
  Message* m = new Message(message, len);

  // Update our state based on receive information from node m->id.

  // If a node's timestamp has increased, we know it was at least alive when this message
  // was sent.
  #ifdef DEBUG
  cout << "receive: "; // update will print out another message.
  #endif
  map<int,int>& df = state.getTimestamp().update(m->getTimestamp());
  for(map<int,int>::iterator it = df.begin();
      it != df.end(); ++it){
    if(it->second > 0){
      heartbeat->reset(it->first);
      #ifdef DEBUG
      cout << "receive: Reset timeout for " << it->first << endl;
      #endif
    }
  }

  externalStates[m->getSenderId()]->updateDeliveryAckList(m->getAcknowledgements());
  state.updateFailedNodes(m->getFailedNodes());

  if(m->getType() == RETRANSREQUEST) {
    int toId = m->getSenderId();
    int fromId;
    int sequenceNumber = m->getSequenceNumber();

    stringstream ss(m->getMessage());
    ss >> fromId;

    #ifdef DEBUG
    cout << "receive: node " << toId << " wants message " << sequenceNumber << " from " << fromId << endl;
    #endif

    Message* m = NULL;

    // Are you asking for one of my messages?
    if (fromId == state.getId()) {
      m = state.getMessage(sequenceNumber);
    } else {
      m = externalStates[fromId]->getMessage(sequenceNumber);
    }
    assert(m != NULL);

    // Update acknowledgements and failed nodes with latest information.
    map<int,int>& acks = m->getAcknowledgements();
    for(map<int,int>::iterator it = acks.begin();
        it != acks.end(); ++it){
      acks[it->first] = externalStates[fromId]->getExternalLatestDeliveredSequenceNumber(it->first);
    }
    m->getFailedNodes().insert(state.getFailedNodes().begin(),state.getFailedNodes().end());

    unicast(toId,*m);
    discard(m);
  }
  else if(m->getType() == MESSAGE) {
    ExternalNodeState& externalState = *externalStates[m->getSenderId()];
    int latestDeliveredSequenceNumber = externalState.getLatestDeliveredSequenceNumber();

    // Have you already delivered this message?
    if (m->getSequenceNumber() <= latestDeliveredSequenceNumber) {
      #ifdef DEBUG
      cout << "receive: already got message " << m->getSequenceNumber() << " from " << m->getSenderId() << endl;
      #endif
      discard(m);
    }
    // This sequence is next in line! Good to deliver.
    else if (m->getSequenceNumber() == latestDeliveredSequenceNumber + 1) {
      #ifdef DEBUG
      cout << "receive: message " << m->getSequenceNumber() << " from " << m->getSenderId() << " ready to deliver." << endl;
      #endif
      // Even though we can deliver it according to FIFO order, we must store to ensure causal order.
      externalState.storeMessage(m);
    } 
    // We're missing some messages...
    else {
      // Have you already gotten this message?
      if(externalState.getMessage(m->getSequenceNumber()) == NULL) {
        // Store this message in the appropriate store.
        externalState.storeMessage(m);
      }

      // Calculate the delta in sequence numbers so we can request a retransmission.
      int delta = m->getSequenceNumber() - latestDeliveredSequenceNumber;

      // Ask for re-transmissions from everyone known to have delivered the messages we need.
      // The reason we ask everyone is that the original sender may have failed.
      for (int i = latestDeliveredSequenceNumber + 1; i < m->getSequenceNumber(); i++) {
        set<int> nodes;
        for (
            map<int, ExternalNodeState*>::iterator it = externalStates.begin();
            it != externalStates.end();
            it++) {

          // Has this process delivered the message we need?
          if (it->second->getExternalLatestDeliveredSequenceNumber(m->getSenderId()) >= m->getSequenceNumber()) {
            nodes.insert(it->first);
          }
        }

        assert(!nodes.empty());

        map<int, int> acknowledgements;
        populateAcknowledgements(acknowledgements, externalStates);

        stringstream ss;
        ss << m->getSenderId();

        // We don't have store this, so it's safe to use stack memory.
        Message rm(
          state.getId(),
          i,
          state.getTimestamp(),
          RETRANSREQUEST,
          ss.str(),
          acknowledgements,
          state.getFailedNodes()
        );

        for (set<int>::iterator it = nodes.begin(); it != nodes.end(); ++it) { 
          unicast(*it, rm);
        }
      }
    }
 
    bool deliveredSomething;
    bool deletedSomething;
    do{
      deliveredSomething = false;
      deletedSomething = true;
      set<Message*> deliverables;
      set<Message*> deletables;

      // Construct a list of all the deliverable messages.
      for(map<int,ExternalNodeState*>::iterator i = externalStates.begin();
          i != externalStates.end();
          ++i){
        ExternalNodeState& extState = *(i->second);
        int latestSeqNum = extState.getLatestDeliveredSequenceNumber();
        for (
            set<Message*>::iterator it = extState.getMessageStore().begin(); 
            it != extState.getMessageStore().end(); 
            ++it) {

          // Is this the message you are expecting?
          if((*it)->getSequenceNumber() == latestSeqNum + 1) {
            deliverables.insert(*it);
            deliveredSomething = true;
          }

          // Are we able to delete this message from the store?
          // Is the sequence number less or equal to all the known externally and internally delivered sequence numbers?
          bool deletable = false;
          if((*it)->getSequenceNumber() <= state.getId()){
            deletable = false;
            for(map<int,ExternalNodeState*>::iterator j = externalStates.begin();
                j != externalStates.end();
                ++i){
              if((*it)->getSequenceNumber() > j->second->getExternalLatestDeliveredSequenceNumber((*it)->getSenderId())){
                deletable = false;
                break;
              }
            }
          }
          if(deletable){
            deletables.insert(*it);
          }
        }
      }

      // Sort the list according to causal order, and deliver.
      vector<Message*> dv;
      copy(deliverables.begin(),deliverables.end(), dv.begin());
      sort(dv.begin(),dv.end());
      for(vector<Message*>::iterator it = dv.begin(); 
          it != dv.end(); 
          ++it) {
          multicast_deliver(*(*it));
      }

      // Delete some messages from the store if possible.
      for(set<Message*>::iterator it = deletables.begin(); 
          it != deletables.end(); 
          ++it) {
          externalState.getMessageStore().erase(*it);
          discard(*it);
      }
    } while(deliveredSomething);
  }
  else { // if(m->type == HEARTBEAT)
    discard(m);
  }
  deliver(source, message);
}
