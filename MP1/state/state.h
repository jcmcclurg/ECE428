#ifndef NODE_STATE_H_
#define NODE_STATE_H_

#include "../message/message.h"
#include "../timestamp/timestamp.h"

#include <vector>
#include <queue>
#include <set>

using namespace std;

class NodeState{
  private:
    //! Unique identifier for this process. 
    int id;
        
    //! Sequence counter.
    int sequenceNumber;

    //! List of all potentially undelivered messages sent by this node. There's an additional set 
    //! tagged to every sequence number to keep track of which nodes have NOT successfully delivered.
    //! We'll occaisonally prune the beginning of this list and removing messages everyone has delivered.
    vector<Message*> messageStore;

    //! Set keeping track of all nodes this process believes to have failed.
    vector<int> failedNodes;

    //! Vector timestamp, with included IDs for easy access.
    Timestamp timestamp;

  public:
    NodeState(int id, vector<int> ids) : id(id), timestamp(id,ids) {}
    NodeState(int id, int* memberIds, int memberCount) : id(id), timestamp(id,memberIds,memberCount) {}
    ~NodeState(){
      for (vector<Message*>::iterator it=messageStore.begin(); it!=messageStore.end(); ++it){
        delete *it;
      }
    }

    int getId() { return id; }
    int getSequenceNumber() { return sequenceNumber; }
    Timestamp& getTimestamp() { return timestamp; }
    void storeMessage(Message* m) {messageStore.push_back(m);}
    vector<Message*> getMessageStore(){ return messageStore;}
    Message* getMessage(int from_id, int sequenceNumber){
      for (vector<Message*>::iterator it=messageStore.begin(); it!=messageStore.end(); ++it){
        if((*it)->getSequenceNumber() == sequenceNumber && (*it)->getSenderId() == from_id){
          return *it;
        }
      }
      return NULL;
    }
    void updateFailedNodes(){
      for (vector<int>::iterator it=failedNodes.begin(); it!=failedNodes.end(); ++it){
        failedNodes.push_back(it);
      }
      sort(failedNodes.begin(),failedNodes.end());
      unique(failedNodes.begin(),failedNodes.end());
    }

    /**
     * Increment the sequence number;
     */
    void sequenceNumberIncrement() {
      sequenceNumber++;
    }
};

class ExternalNodeState{
  private:
    //! Unique identifier for this external process. 
    int id;

    //! Sequence number of the last message delivered from this node.
    int latestDeliveredSequenceNumber;

    //! Sequence number of the messages this node has delivered
    map<int,int> deliveryAckList;

  public:
    ExternalNodeState(int id) : id(id) {}

    int getId() const { return id; }
    int getLatestDeliveredSequenceNumber() const { return latestDeliveredSequenceNumber; }
    void updateLatestDeliveredSequenceNumber(int num){
      latestDeliveredSequenceNumber = num;
    }
    void updateDeliveryAckList(map<int,int> list){
    for (
        map<int, int>::iterator it = list.begin();
        it != list.end();
        it++) {
      deliveryAckList[it->first] = max(deliveryAckList[it->first], it->second);
    }}
    int getExternalLatestLatestSequenceNumber(int id){ return deliveryAckList[id]; }
};

class GlobalState {
  NodeState state;
  map<int, ExternalNodeState*> externalStates;

  GlobalState(int ownId, int* memberIds, int memberCount) : state(ownId,memberIds,memberCount) {
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
