#include "mp1.h"
#include "rmcast.h"

//! The global state information
GlobalState* globalState = NULL;
Heartbeat* heartbeat;
char encodeBuffer[100];

static void heartbeat_failure(int sig, siginfo_t *si, void *uc) {
  int failedId = si->si_value.sival_int;
  globalState->state.getFailedNodes().insert(failedId);
}

static void heartbeat_send() {
  NodeState& state = globalState->state;
  map<int, ExternalNodeState*> externalStates = globalState->externalStates;

  #ifdef DEBUG
    cout << "Sending heartbeat at time " << state.getTimestamp() << endl;
  #endif

  map<int, int> acknowledgements;
  populateAcknowledgements(acknowledgements, externalStates);

  // Wrap the message in a Message object.
  Message* m = new Message(
    state.getId(),
    state.getSequenceNumber(),
    state.getTimestamp(),
    HEARTBEAT,
    string(),
    acknowledgements,
    state.getFailedNodes()
  );

  // Unicast all the messages.
  for (
      map<int, ExternalNodeState*>::iterator it = externalStates.begin();
      it != externalStates.end();
      it++) {
      unicast(it->first, *m);
  }
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
  #ifdef DEBUG
    cout << "Delivering " << m << endl;
  #endif
  if (m.getSenderId() != globalState->state.getId()) {
    globalState->externalStates[m.getSenderId()]->latestDeliveredSequenceNumberIncrement();
  }
  deliver(m.getSenderId(), m.getMessage().c_str());
}

void populateAcknowledgements(
      map<int, int>& acknowledgements, 
      map<int, ExternalNodeState*>& externalStates) {

  // Messages we have delivered from the other nodes.
  for (
      map<int, ExternalNodeState*>::iterator it = externalStates.begin();
      it != externalStates.end();
      it++) {

    acknowledgements[it->first] = it->second->getLatestDeliveredSequenceNumber();
  }
  // Messages we have delivered from ourselves.
  acknowledgements[globalState->state.getId()] = globalState->state.getSequenceNumber();
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
void multicast(const char *message) {
  initIfNecessary();

  NodeState& state = globalState->state;
  map<int, ExternalNodeState*> externalStates = globalState->externalStates;

  // Increment the timestamp before you do anything else.
  state.getTimestamp().step();
  state.sequenceNumberIncrement();
  #ifdef DEBUG
    cout << "Sending message " << state.getSequenceNumber() << " at time " << state.getTimestamp() << endl;
  #endif

  map<int, int> acknowledgements;
  populateAcknowledgements(acknowledgements, externalStates);

  // Wrap the message in a Message object.
  Message* m = new Message(
    state.getId(),
    state.getSequenceNumber(),
    state.getTimestamp(),
    MESSAGE,
    string(message),
    acknowledgements,
    state.getFailedNodes()
  );

  // Deliver to yourself first, so that if you fail before sending it to everyone, the message can still 
  // get re-transmitted by someone else without violating the R-multicast properties.
  // multicast_deliver increments either our sequence number or the latest delivered seq number of external.
  multicast_deliver(*m);

  // Add to the sent messages list so you can grab this message again in case a retransmission is needed.
  state.storeMessage(m);

  // Unicast all the messages.
  for (
      map<int, ExternalNodeState*>::iterator it = externalStates.begin();
      it != externalStates.end();
      it++) {
      unicast(it->first, *m);
  }
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

  #ifdef DEBUG
  cout << "Received a ";
  if(m->getType() == RETRANSREQUEST){
    cout << "retransmission request";
  }
  else if(m->getType() == MESSAGE){
    cout << "message";
  }
  else{ // if(m->getType() == HEARTBEAT)
    cout << "heartbeat";
  }
  cout << " from " << source << " at " << m->getTimestamp() << endl;
  #endif

  // Update our state based on receive information from node m->id.

  // If a node's timestamp has increased, we know it was at least alive when this message
  // was sent.
  map<int,int>& df = state.getTimestamp().update(m->getTimestamp());
  for(map<int,int>::iterator it = df.begin();
      it != df.end(); ++it){
    if(it->second > 0){
      heartbeat->reset(it->first);
      #ifdef DEBUG
      cout << "Reset timeout for " << it->first << endl;
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
    discard(m);

    #ifdef DEBUG
    cout << "Node " << toId << " wants message " << sequenceNumber << " from " << fromId << ". Checking..." << endl;
    #endif

    Message* m = NULL;
    // Are you asking for one of my messages or for someone else's?
    if (fromId == state.getId()) {
      m = state.getMessage(sequenceNumber);
    } else {
      m = externalStates[fromId]->getMessage(sequenceNumber);
    }
    if(m != NULL){
      unicast(toId,*m);
      #ifdef DEBUG
      cout << "Retransmitted " << *m << endl;
      #endif
    }
    #ifdef DEBUG
    else{
      cout << "Couldn't find that message." << endl;
    }
    #endif
  }
  else if(m->getType() == MESSAGE) {
    ExternalNodeState& externalState = *externalStates[m->getSenderId()];
    int latestDeliveredSequenceNumber = externalState.getLatestDeliveredSequenceNumber();

    // Have you already delivered this message?
    if (m->getSequenceNumber() <= latestDeliveredSequenceNumber || externalState.getMessage(m->getSequenceNumber()) != NULL) {
      #ifdef DEBUG
      cout << "Already received message " << m->getSequenceNumber() << " from " << m->getSenderId() << ". Discarding." << endl;
      #endif
      discard(m);
    }

    // This sequence is next in line! Good to deliver.
    else if (m->getSequenceNumber() == latestDeliveredSequenceNumber + 1) {
      #ifdef DEBUG
      cout << "Message " << m->getSequenceNumber() << " from " << m->getSenderId() << " ready to deliver." << endl;
      #endif
      // Even though we can deliver it according to FIFO order, we must store to ensure causal order.
      externalState.storeMessage(m);
    }

    // We're missing some messages from this particular node.
    else {
      #ifdef DEBUG
      cout << "Got message " << m->getSequenceNumber() << " from " << m->getSenderId() << ". Was expecting " << latestDeliveredSequenceNumber + 1 << "." << endl;
      #endif
      // Store this message in the appropriate store.
      externalState.storeMessage(m);

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

        // This should at least contain the original sender.
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
          #ifdef DEBUG
          cout << "Asking " << *it << " to retransmit message " << i << " from " << m->getSenderId() << endl;
          #endif
          unicast(*it, rm);
        }
      }
    }
 


    #ifdef DEBUG
      cout << "Attempting to deliver messages..." << endl;
    #endif
    bool deliveredSomething;
    do{
      deliveredSomething = false;
      set<Message*> undelivered;

      // Construct a list of all the undelivered messages. It is known that all
      // undelivered messages are either concurrent with or happen-after delivered
      // messages, so the delivered messages need not be considered during the
      // causal ordering process.
      for(map<int,ExternalNodeState*>::iterator i = externalStates.begin();
          i != externalStates.end();
          ++i){
        ExternalNodeState& extState = *(i->second);
        int latestSeqNum = extState.getLatestDeliveredSequenceNumber();
        set<Message*> store = extState.getMessageStore(); 

        for(set<Message*>::iterator it = store.begin();
            it != store.end(); 
            ++it) {
          // Has this message been delivered yet?
          if((*it)->getSequenceNumber() > latestSeqNum) {
            #ifdef DEBUG
            cout << "Undelivered: " << *(*it) << endl;
            #endif
            Message* t = *it;
            undelivered.insert(t);
          }
          #ifdef DEBUG
          else{
            cout << "Delivered: " << *(*it) << endl;
          }
          #endif
        }
      }

      if(undelivered.size() > 0){
        // Sort the undelivered list according to causal order, and deliver.
        #ifdef DEBUG
        cout << "Sorting undelivered messages by causal order..." << endl;
        #endif
        vector<Message*> dv;
        dv.resize(undelivered.size());
        copy(undelivered.begin(),undelivered.end(), dv.begin());
        sort(dv.begin(),dv.end());
        Message* first = dv[0];
        Timestamp& lastTried = first->getTimestamp();
        for(vector<Message*>::iterator it = dv.begin(); 
            it != dv.end(); 
            ++it) {

          // Deliver messages if you can.
          ExternalNodeState& extState = *(externalStates[(*it)->getSenderId()]);
          if((*it)->getSequenceNumber() == extState.getLatestDeliveredSequenceNumber() + 1){
            deliveredSomething = true;
            multicast_deliver(*(*it));
            lastTried = (*it)->getTimestamp();
          }

          // Stop delivering messages if doing so would violate causality.
          else if((*it)->getTimestamp().compare(lastTried) == AFTER){
            #ifdef DEBUG
            cout << "Waiting to deliver " << *it << endl;
            #endif
            break;
          }
        }
      }
      #ifdef DEBUG
      else{
        cout << "No undelivered messages remaining." << endl;
      }
      #endif
    } while(deliveredSomething);
    #ifdef DEBUG
    cout << "Finished message delivery attempt." << endl << "Attempting to free up message store..." << endl;
    #endif

    // Go through all the nodes.
    for(map<int,ExternalNodeState*>::iterator i = externalStates.begin();
        i != externalStates.end();
        ++i){
      // Go through all this node's messages.
      ExternalNodeState& extState = *(i->second);
      int latestSeqNum = extState.getLatestDeliveredSequenceNumber();
      set<Message*> store = externalState.getMessageStore();
      for(set<Message*>::iterator it = store.begin(); 
          it != store.end(); 
          ++it) {
        bool deletable = false;

        // Have we delivered this message?
        if((*it)->getSequenceNumber() <= state.getId()){
          deletable = true;
          // Has everyone else delivered this message?
          for(map<int,ExternalNodeState*>::iterator j = externalStates.begin();
              j != externalStates.end();
              ++j){
            if((*it)->getSequenceNumber() > j->second->getExternalLatestDeliveredSequenceNumber((*it)->getSenderId())){
              deletable = false;
              break;
            }
          }
        }

        // Are we able to delete this message from the store?
        if(deletable){
          #ifdef DEBUG
          cout << "Deleting: " << *(*it) << endl;
          #endif
          store.erase(*it);
          discard(*it);
        }
      }
    }
    #ifdef DEBUG
    cout << "Finished cleaning message store." << endl;
    #endif
  }
  else { // if(m->type == HEARTBEAT)
    discard(m);
  }
}
