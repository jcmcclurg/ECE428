#ifndef NODE_STATE_H_
#define NODE_STATE_H_

#include "../message/message.h"
#include "../timestamp/timestamp.h"

#include <algorithm>
#include <queue>
#include <set>

using namespace std;

// Represents our current process.
class Node{
  private:
    //! Unique identifier for this process. 
    int id;
        
    //! Sequence counter.
    int sequenceNumber;

    //! Set keeping track of all nodes this process believes to have failed.
    set<int> failedNodes;

    //! Vector timestamp, with included IDs for easy access.
    Timestamp timestamp;

  public:
    Node(int id, int* memberIds, int memberCount);
    Node(int id, set<int> ids) : id(id), sequenceNumber(0), timestamp(id, ids) {}

    int getId() { return id; }
    int getSequenceNumber() { return sequenceNumber; }
    Timestamp& getTimestamp() { return timestamp; }
    set<int>& getFailedNodes() { return failedNodes; }

    void updateFailedNodes(set<int>& otherFailedNodes);
    void sequenceNumberIncrement() { sequenceNumber++; }
};

// Represents data we need to know about other processes.
class ExternalNode {
  private:
    //! Unique identifier for this external process. 
    int id;

    //! Sequence number of the last message delivered from this node.
    int lastSequenceNumber;

    //! Sequence number of the messages this node has delivered
    map<int, int> deliveryAckList;

  public:
    void updateDeliveryAckList(map<int,int>& list);

    ExternalNode(int id) : id(id), lastSequenceNumber(0) {}

    int getId() const { return id; }
    int getLastSequenceNumber() const { return lastSequenceNumber; }
    void lastSequenceNumberIncrement() { lastSequenceNumber++; }
    int getExternalLastSequenceNumber(int id) { return deliveryAckList[id]; }
};

struct GlobalState {
  public:
    //! List of all messages sent by everyone.
    set<Message*> messageStore;

    //! Node bookkeeping.
    Node node;
    map<int, ExternalNode*> externalNodes;

    GlobalState(int ownId, int* memberIds, int memberCount);
    ~GlobalState();

    Message* getMessage(int fromId, int sequenceNumber);
    void storeMessage(Message* m);
};

#endif
