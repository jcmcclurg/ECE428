#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <map>
#include <iostream>
#include <vector>
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

//! Forward declaration for our multicast function.
void multicast(const char *message, MessageType type);

static void heartbeat_failure(int sig, siginfo_t *si, void *uc) {
  int failedId = si->si_value.sival_int;
  globalState->state.getFailedNodes().insert(failedId);
}

static void heartbeat_send() {
  multicast("xoxo", HEARTBEAT);
}

/**
 * IMPORTANT: Assumes that the group membership will not grow during runtime.
 */
void multicast_init(void) {
  unicast_init();
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

/**
 * Reliable multicast implementation.
 */
void multicast(const char *message, MessageType type) {
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
      string em = m->getEncodedMessage();
      usend(it->first, em.c_str(), em.size());
    }
  }

  // Increment your own sent message sequence number.
  // state.sequenceNumberIncrement();
}

void multicast(const char *message) {
  multicast(message, MESSAGE);
}

void mcast_join(int member) {
  printf("%d friggin joined.", member);
}

void discard(Message* m) {
  // Free up some memory or something?
}

void receive(int source, const char *message, int len) {
  assert(source != my_id);

  NodeState& state = globalState->state;
  map<int, ExternalNodeState*> externalStates = globalState->externalStates;

  // Increment the timestamp before you do anything else.
  state.getTimestamp().step();

  // De-serialize the message into a new Message object. 
  Message* m = new Message(string(message));

  // Update our state based on receive information from node m->id.
  state.getTimestamp().update(m->getTimestamp());
  externalStates[m->getSenderId()]->updateDeliveryAckList(m->getAcknowledgements());
  state.updateFailedNodes(m->getFailedNodes());

  if(m->getType() == RETRANSREQUEST) {
    int fromId = m->getSenderId();
    int toId;
    int sequenceNumber = m->getSequenceNumber();

    stringstream ss(m->getMessage());
    ss >> toId;

    Message* m = NULL;

    // Are you asking for one of my messages?
    if (fromId == state.getId()) {
      m = state.getMessage(sequenceNumber);
    } else {
      m = externalStates[fromId]->getMessage(sequenceNumber);
    }
    assert(m != NULL);

    // TODO: UPDATE FIELDS IN RETRANSMISSION MESSAGE

    string em = m->getEncodedMessage();
    usend(toId, em.c_str(), em.size());

    discard(m);
  }
  else if(m->getType() == MESSAGE) {
    ExternalNodeState& externalState = *externalStates[m->getSenderId()];
    int latestDeliveredSequenceNumber = externalState.getLatestDeliveredSequenceNumber();

    // Have you already delivered this message?
    if (m->getSequenceNumber() <= latestDeliveredSequenceNumber) {
      discard(m);
    }
    // This sequence is next in line! Good to deliver.
    else if (m->getSequenceNumber() == latestDeliveredSequenceNumber + 1) {
      multicast_deliver(*m);

      // Maybe we can deliver some more messages before we're done.
      for (
          vector<Message*>::iterator it = externalState.getMessageStore().begin(); 
          it != externalState.getMessageStore().end(); 
          ++it) {

        // Is this the message you are expecting?
        if((*it)->getSequenceNumber() == latestDeliveredSequenceNumber + 1) {
          multicast_deliver(*(*it));
        }
      }
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
        vector<int> nodes;
        for (
            map<int, ExternalNodeState*>::iterator it = externalStates.begin();
            it != externalStates.end();
            it++) {

          // Has this process delivered the message we need?
          if (it->second->getExternalLatestDeliveredSequenceNumber(m->getSenderId()) >= m->getSequenceNumber()) {
            nodes.push_back(it->first);
          }
        }

        assert(!nodes.empty());

        map<int, int> acknowledgements;
        populateAcknowledgements(acknowledgements, externalStates);

        stringstream ss;
        ss << i;

        // We don't have store this, so it's safe to use stack memory.
        Message rm(
          state.getId(),
          state.getSequenceNumber(),
          state.getTimestamp(),
          RETRANSREQUEST,
          ss.str(),
          acknowledgements,
          state.getFailedNodes()
        );

        for (vector<int>::iterator it = nodes.begin(); it != nodes.end(); ++it) { 
          string em = rm.getEncodedMessage();
          usend(*it, em.c_str(), em.size());
        }
      }
    }
  }
  else { // if(m->type == HEARTBEAT)
    discard(m);
  }

  state.getTimestamp().update(m->getTimestamp());
  deliver(source, message);
}