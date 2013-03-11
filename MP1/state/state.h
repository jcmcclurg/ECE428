#ifndef NODE_STATE_H_
#define NODE_STATE_H_

#include "../message/message.h"
#include "../timestamp/timestamp.h"

#include <algorithm>
#include <queue>
#include <set>

using namespace std;

// Represents our current process.
class NodeState{
  private:
    //! Unique identifier for this process. 
    int id;
        
    //! Sequence counter.
    int sequenceNumber;

    //! List of all messages sent by this node.
    set<Message*> messageStore;

    //! Set keeping track of all nodes this process believes to have failed.
    set<int> failedNodes;

    //! Vector timestamp, with included IDs for easy access.
    Timestamp timestamp;

  public:
    NodeState(int id, set<int> ids) : id(id), timestamp(id, ids) {}
    NodeState(int id, int* memberIds, int memberCount) : id(id), timestamp(id, memberIds, memberCount) {}
    ~NodeState(){
      for (set<Message*>::iterator it=messageStore.begin(); it!=messageStore.end(); ++it){
        delete *it;
      }
    }

    int getId() { return id; }
    int getSequenceNumber() { return sequenceNumber; }
    Timestamp& getTimestamp() { return timestamp; }
    set<Message*>& getMessageStore() { return messageStore; }
    set<int>& getFailedNodes() { return failedNodes; }

    Message* getMessage(int sequenceNumber) {
      for (set<Message*>::iterator it = messageStore.begin(); it != messageStore.end(); ++it){
        if((*it)->getSequenceNumber() == sequenceNumber) {
          return *it;
        }
      }
      return NULL;
    }
    void storeMessage(Message* m) { messageStore.insert(m); }

    void updateFailedNodes(set<int>& otherFailedNodes) {
      failedNodes.insert(otherFailedNodes.begin(), otherFailedNodes.end());
    }

    /**
     * Increment the sequence number
     */
    void sequenceNumberIncrement() {
      sequenceNumber++;
    }
};

// Represents data we need to know about other processes.
class ExternalNodeState{
  private:
    //! Unique identifier for this external process. 
    int id;

    //! Sequence number of the last message delivered from this node.
    int latestDeliveredSequenceNumber;

    //! List of all messages sent by this node.
    set<Message*> messageStore;

    //! Sequence number of the messages this node has delivered
    map<int, int> deliveryAckList;

  public:
    ExternalNodeState(int id) : id(id) {}

    int getId() const { return id; }
    int getLatestDeliveredSequenceNumber() const { return latestDeliveredSequenceNumber; }
    set<Message*> getMessageStore(){ return messageStore; }

    /**
     * Increment the latest sequence number
     */
    void latestDeliveredSequenceNumberIncrement() {
      latestDeliveredSequenceNumber++;
    }

    void updateDeliveryAckList(map<int,int> list){
      #ifdef DEBUG
      cout << "external state[" << id << "] deliveryAckList = {";
      #endif
      for (
          map<int, int>::iterator it = list.begin();
          it != list.end();
          it++) {

        deliveryAckList[it->first] = max(deliveryAckList[it->first], it->second);
        #ifdef DEBUG
        cout << it->first << "=" << it->second << ",";
        #endif
      }
      #ifdef DEBUG
      cout << "}" << endl; 
      #endif
    }

    Message* getMessage(int sequenceNumber) {
      for (set<Message*>::iterator it = messageStore.begin(); it != messageStore.end(); ++it){
        if((*it)->getSequenceNumber() == sequenceNumber) {
          return *it;
        }
      }
      return NULL;
    }
    void storeMessage(Message* m) { messageStore.insert(m); }

    int getExternalLatestDeliveredSequenceNumber(int id) { return deliveryAckList[id]; }
};

struct GlobalState {
  NodeState state;
  map<int, ExternalNodeState*> externalStates;

  GlobalState(int ownId, int* memberIds, int memberCount) : state(ownId, memberIds, memberCount) {
    for (int i = 0; i < memberCount; ++i) {
      if (memberIds[i] != ownId) {
        externalStates[memberIds[i]] = new ExternalNodeState(memberIds[i]);
      }
    } 
  }

  ~GlobalState() {
    for (
        map<int, ExternalNodeState*>::iterator it = externalStates.begin();
        it != externalStates.end();
        it++) {

      delete it->second;
    }
  }
};

#endif
