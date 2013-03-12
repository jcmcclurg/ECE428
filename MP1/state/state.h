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
    NodeState(int id, int* memberIds, int memberCount);
    ~NodeState();
    Message* getMessage(int sequenceNumber);
    void updateFailedNodes(set<int>& otherFailedNodes);

    NodeState(int id, set<int> ids) : id(id), sequenceNumber(0), timestamp(id, ids) {}
    int getId() { return id; }
    int getSequenceNumber() { return sequenceNumber; }
    Timestamp& getTimestamp() { return timestamp; }
    set<Message*>& getMessageStore() { return messageStore; }
    set<int>& getFailedNodes() { return failedNodes; }
    void storeMessage(Message* m) { messageStore.insert(m); }

    void sequenceNumberIncrement() { sequenceNumber++; }
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
    void updateDeliveryAckList(map<int,int>& list);
    Message* getMessage(int sequenceNumber);

    ExternalNodeState(int id) : id(id), latestDeliveredSequenceNumber(0) {}
    ~ExternalNodeState();
    int getId() const { return id; }
    int getLatestDeliveredSequenceNumber() const { return latestDeliveredSequenceNumber; }
    set<Message*> getMessageStore(){ return messageStore; }
    void latestDeliveredSequenceNumberIncrement() { latestDeliveredSequenceNumber++; }
    int getExternalLatestDeliveredSequenceNumber(int id) { return deliveryAckList[id]; }
    void storeMessage(Message* m) { messageStore.insert(m); }
};

struct GlobalState {
  public:
  NodeState state;
  map<int, ExternalNodeState*> externalStates;

  GlobalState(int ownId, int* memberIds, int memberCount);
  ~GlobalState();
};

#endif
