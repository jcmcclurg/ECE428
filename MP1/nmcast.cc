#include <climits>
#include "mp1.h"
#include "rmcast.h"

//! The global state information
GlobalState* globalState = NULL;
Heartbeat* heartbeat;

char encodeBuffer[1000];

static void heartbeat_failure(int sig, siginfo_t *si, void *uc) {
  int failedId = si->si_value.sival_int;
  #ifdef DEBUG
  cout << "Node " << failedId << " has failed." << endl;
  #endif
  globalState->node.getFailedNodes().insert(failedId);
}

static void heartbeat_send() {
  Node& node = globalState->node;
  map<int, ExternalNode*>& externalNodes = globalState->externalNodes;

  #ifdef DEBUG
  cout << "Sending heartbeat at time " << node.getTimestamp() << endl;
  #endif

  map<int, int> acknowledgements;
  populateAcknowledgements(acknowledgements);

  // Wrap the heartbeat message in a Message object.
  Message* m = new Message(
    node.getId(),
    node.getSequenceNumber(),
    node.getTimestamp(),
    HEARTBEAT,
    string(),
    acknowledgements,
    node.getFailedNodes()
  );

  // Unicast all the messages.
  for (map<int, ExternalNode*>::iterator it = externalNodes.begin();
      it != externalNodes.end();
      ++it) {
    
    unicast(it->first, *m);
  }
}

void unicast(int to, Message& m) {
  Node& node = globalState->node;
  if (node.getFailedNodes().find(to) == node.getFailedNodes().end()) {
    int sz = m.getEncodedMessage(encodeBuffer);
    usend(to, encodeBuffer, sz);
    #ifdef DEBUG
    cout << "Sending to " << to << endl;
    #endif
  }
  #ifdef DEBUG
  else{
    cout << "Not sending to " << to << " because it has failed." << endl;
  }
  #endif
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
  if (m.getSenderId() != globalState->node.getId()) {
    globalState->externalNodes[m.getSenderId()]->lastSequenceNumberIncrement();
  }
  deliver(m.getSenderId(), m.getMessage().c_str());
}

void populateAcknowledgements(map<int, int>& acknowledgements) {
  map<int, ExternalNode*>& externalNodes = globalState->externalNodes;

  // Messages we have delivered from the other nodes.
  for (map<int, ExternalNode*>::iterator it = externalNodes.begin();
      it != externalNodes.end();
      ++it) {

    acknowledgements[it->first] = it->second->getLastSequenceNumber();
  }
  // Messages we have delivered from ourselves.
  acknowledgements[globalState->node.getId()] = globalState->node.getSequenceNumber();
}

void initIfNecessary(){
  if(globalState == NULL){
    globalState = new GlobalState(my_id, mcast_members, mcast_num_members);
    heartbeat = new Heartbeat(
      my_id,
      mcast_members, 
      mcast_num_members, 
      10*HEARTBEAT_MS + MAXDELAY, 
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

  Node& node = globalState->node;
  map<int, ExternalNode*>& externalNodes = globalState->externalNodes;

  // Increment the timestamp and sequence number before you do anything else.
  node.getTimestamp().step();
  node.sequenceNumberIncrement();
  #ifdef DEBUG
  cout << "Sending message " << node.getSequenceNumber() << " at time " << node.getTimestamp() << endl;
  #endif

  map<int, int> acknowledgements;
  populateAcknowledgements(acknowledgements);

  // Wrap the message in a Message object.
  Message* m = new Message(
    node.getId(),
    node.getSequenceNumber(),
    node.getTimestamp(),
    MESSAGE,
    string(message),
    acknowledgements,
    node.getFailedNodes()
  );

  // Deliver to yourself first, so that if you fail before sending it to everyone, the message can still 
  // get re-transmitted by someone else without violating the R-multicast properties.
  // multicast_deliver increments either our sequence number or the latest delivered seq number of external.
  multicast_deliver(*m);

  // Add to the sent messages list so you can grab this message again in case a retransmission is needed.
  globalState->storeMessage(m);

  // Unicast all the messages.
  for (map<int, ExternalNode*>::iterator it = externalNodes.begin();
      it != externalNodes.end();
      ++it) {

    unicast(it->first, *m);
  }

  //processUndelivered();
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

void retransmissionRequests(){
  map<int, ExternalNode*> externalNodes = globalState->externalNodes;
  Node& node = globalState->node;
  set<Message*>& store = globalState->messageStore;

  //map<int,set<int>> missingSeqNums; // TODO: ask for all missing sequence numbers, instead of just the lowest
  //map<int,int> maxSequenceNums;
  map<int,int> minSequenceNums;

  for (set<Message*>::iterator it = store.begin();
      it != store.end(); 
      ++it) {

    if((*it)->getSenderId() != node.getId()){
    //maxSequenceNums[it->first] = INT_MIN;
      minSequenceNums[(*it)->getSenderId()] = INT_MAX;
    }
  }

  // Loop through all the stored messages, to find the range of messages to look for
  for (set<Message*>::iterator it = store.begin();
      it != store.end(); 
      ++it) {

    //maxSequenceNums[m->getSenderId()] = max(maxSequenceNums[m->getSenderId()], m->getSequenceNumber());
    if((*it)->getSenderId() != node.getId()){
      minSequenceNums[(*it)->getSenderId()] = min(minSequenceNums[(*it)->getSenderId()], (*it)->getSequenceNumber());
    }
  }

  for(map<int, int>::iterator j = minSequenceNums.begin();
      j != minSequenceNums.end();
      ++j){
      int lastSequenceNumber = globalState->externalNodes[j->first]->getLastSequenceNumber();

      for (int i = lastSequenceNumber + 1; i < j->second; i++) {
        set<int> deliveredNodes;
        for (map<int, ExternalNode*>::iterator it = externalNodes.begin();
            it != externalNodes.end();
            it++) {

          ExternalNode* groupMember = it->second;

          // Has this process delivered the message we need?
          if (groupMember->getExternalLastSequenceNumber(j->first) >= i) {
            deliveredNodes.insert(it->first);
          }
        }

        // This should at least contain the original sender.
        #ifdef DEBUG
        cout << j->first << " had delivered " << lastSequenceNumber << ". We have " << j->second <<" in the store." << endl;
        #endif
        assert(!deliveredNodes.empty());

        map<int, int> acknowledgements;
        populateAcknowledgements(acknowledgements);

        stringstream ss;
        ss << j->first;

        // We don't have store this, so it's safe to use stack memory.
        Message rm(
          node.getId(),
          i,
          node.getTimestamp(),
          RETRANSREQUEST,
          ss.str(),
          acknowledgements,
          node.getFailedNodes()
        );

        for (set<int>::iterator it = deliveredNodes.begin(); it != deliveredNodes.end(); ++it) {
          #ifdef DEBUG
          cout << "Asking " << *it << " to retransmit message " << i << " from " << j->first << endl;
          #endif
          unicast(*it, rm);
        }
      }
  }
}

void cleanMessageStore() {
  map<int, ExternalNode*>& externalNodes = globalState->externalNodes;
  set<Message*>& store = globalState->messageStore; 

  // Go through all messages.
  for (set<Message*>::iterator it = store.begin(); 
      it != store.end(); 
      ++it) {

    Message* m = *it;
    bool deletable = false;

    // This is from myself. I've obviously delivered it.
    if (m->getSenderId() == globalState->node.getId()) {
      deletable = true;
    } else {
      ExternalNode* messageOrigin = globalState->externalNodes[m->getSenderId()];
      int lastSequenceNumber = messageOrigin->getLastSequenceNumber();

      // Have we delivered this message?
      if (m->getSequenceNumber() <= lastSequenceNumber) {
        deletable = true;
      }
    }

    // Has everyone else delivered this message?
    for(map<int, ExternalNode*>::iterator jit = externalNodes.begin();
        jit != externalNodes.end();
        ++jit) {

      ExternalNode* groupMember = jit->second;
      if(m->getSequenceNumber() > groupMember->getExternalLastSequenceNumber(m->getSenderId())) {
        deletable = false;
        break;
      }
    }

    // Are we able to delete this message from the store?
    if (deletable) {
      #ifdef DEBUG
      cout << "Deleting: " << *(*it) << endl;
      #endif

      store.erase(m);
      delete m;
    }
  }
  #ifdef DEBUG
  cout << "Finished cleaning message store." << endl;
  #endif
}

void receive(int source, const char *message, int len) {
  initIfNecessary();
  assert(source != my_id);

  Node& node = globalState->node;
  map<int, ExternalNode*> externalNodes = globalState->externalNodes;

  // Increment the timestamp before you do anything else.
  node.getTimestamp().step();

  // De-serialize the message into a new Message object. 
  Message* m = new Message(message, len);
  ExternalNode* messageOrigin = externalNodes[m->getSenderId()];

  #ifdef DEBUG
  cout << "Received a ";
  if (m->getType() == RETRANSREQUEST) {
    cout << "retransmission request";
  } else if(m->getType() == MESSAGE) {
    cout << "message";
  } else { // if(m->getType() == HEARTBEAT)
    cout << "heartbeat";
  }
  cout << " from " << source << " at " << m->getTimestamp() << endl;
  #endif

  // Update our state based on received information from this message's sender.
  // If a node's timestamp has increased, we know it was at least alive when this message was sent.
  map<int,int>& diff = node.getTimestamp().update(m->getTimestamp());
  for (map<int,int>::iterator it = diff.begin(); it != diff.end(); ++it) {
    if (it->second > 0) {
      heartbeat->reset(it->first);
      #ifdef DEBUG
      cout << "Reset timeout for " << it->first << endl;
      #endif
    }
  }
  messageOrigin->updateDeliveryAckList(m->getAcknowledgements());
  //node.updateFailedNodes(m->getFailedNodes());

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

    Message* m = globalState->getMessage(fromId, sequenceNumber);
    if (m != NULL) {
      unicast(toId, *m);
      #ifdef DEBUG
      cout << "Retransmitted " << *m << endl;
      #endif
    }
    #ifdef DEBUG
    else {
      cout << "Couldn't find that message." << endl;
    }
    #endif
  }
  else if(m->getType() == MESSAGE) {
    int lastSequenceNumber = messageOrigin->getLastSequenceNumber();

    // Have you already received this message?
    if (m->getSequenceNumber() <= lastSequenceNumber || 
        globalState->getMessage(m->getSenderId(), m->getSequenceNumber()) != NULL) {

      #ifdef DEBUG
      cout << "Already received message " << m->getSequenceNumber() << " from " << m->getSenderId() << ". Discarding." << endl;
      #endif
      discard(m);
    }
    else{ 
      #ifdef DEBUG
      if (m->getSequenceNumber() == lastSequenceNumber + 1) {
        // Even though we can deliver it according to FIFO order, we must store to ensure causal order.
        cout << "Message " << m->getSequenceNumber() << " from " << m->getSenderId() << " ready to deliver." << endl;
      }
      else{
        cout << "Got message " << m->getSequenceNumber() << " from " << m->getSenderId() << ". Was expecting " << lastSequenceNumber + 1 << "." << endl;
      }
      #endif
      globalState->storeMessage(m);

      #ifdef DEBUG
      cout << "Requesting retransmissions..." << endl;
      #endif
      retransmissionRequests();
 
      #ifdef DEBUG
      cout << "Attempting to deliver messages..." << endl;
      #endif
      processUndelivered();

      #ifdef DEBUG
      cout << "Finished message delivery attempt." << endl << "Attempting to free up message store..." << endl;
      #endif
      cleanMessageStore();
    }
  }
  else { // if(m->type == HEARTBEAT)
    retransmissionRequests();
    discard(m);
  }
}
  
void processUndelivered() {
  // Construct a list of all the undelivered messages. It is known that all
  // undelivered messages are either concurrent with or happen-after delivered
  // messages, so the delivered messages need not be considered during the
  // causal ordering process.
  set<Message*>& store = globalState->messageStore; 
  vector<Message*> undelivered;

  for (set<Message*>::iterator it = store.begin();
      it != store.end(); 
      ++it) {

    Message* m = *it;

    // Only check messages from external nodes
    if (m->getSenderId() != globalState->node.getId()) {
      ExternalNode* messageOrigin = globalState->externalNodes[m->getSenderId()];    
      // Has this message been delivered yet?
      if (m->getSequenceNumber() > messageOrigin->getLastSequenceNumber()) {
        #ifdef DEBUG
        cout << "Undelivered: " << *m << endl;
        #endif
        undelivered.push_back(m);
      }
      #ifdef DEBUG
      else{
        cout << "Delivered: " << *m << endl;
      }
      #endif
    }
  }

  if (undelivered.size() > 0) {
    // Sort the undelivered list according to causal order, and deliver.
    #ifdef DEBUG
    cout << "Sorting undelivered messages by causal order..." << endl;
    #endif
    sort(undelivered.begin(), undelivered.end());

    for(vector<Message*>::iterator it = undelivered.begin(); 
        it != undelivered.end(); 
        ++it) {

      Message* undeliveredMessage = *it;
      ExternalNode* undeliveredOrigin = globalState->externalNodes[undeliveredMessage->getSenderId()];

      // Check to see if we can deliver this message now.
      if (undeliveredMessage->getSequenceNumber() == undeliveredOrigin->getLastSequenceNumber() + 1) {
        multicast_deliver(*undeliveredMessage);
      }
    }
  }
  #ifdef DEBUG
  else{
    cout << "No undelivered messages remaining." << endl;
  }
  #endif
}
