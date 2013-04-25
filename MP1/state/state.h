#ifndef NODE_STATE_H_
#define NODE_STATE_H_

#include "../message/message.h"
#include "../timestamp/timestamp.h"

#include <algorithm>
#include <queue>
#include <set>

using namespace std;

//! Represents our current process.
class Node {
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

    //! Merges our failed node set with an external set of failed nodes.
    //! 
    //! @param otherFailedNodes The external set of failed nodes to merge.
    void updateFailedNodes(set<int>& otherFailedNodes);

    //! Increments our own sequence number.
    void sequenceNumberIncrement() { sequenceNumber++; }
};

//! Represents data we need to know about other processes.
class ExternalNode {
  private:
    //! Unique identifier for this external process. 
    int id;

    //! Sequence number of the last message delivered from this node.
    int lastSequenceNumber;

    //! A map from process ID k to the last sequence number this external process 
    //! has delivered from process k.
    map<int, int> deliveryAckList;

  public:
    ExternalNode(int id) : id(id), lastSequenceNumber(0) {}

    int getId() const { return id; }
    int getLastSequenceNumber() const { return lastSequenceNumber; }
    void lastSequenceNumberIncrement() { lastSequenceNumber++; }
    int getExternalLastSequenceNumber(int id) { return deliveryAckList[id]; }

    //! Merges our delivery ack list with an external list of delivery acks.
    //! 
    //! @param list The external list of delivery acks to merge.
    void updateDeliveryAckList(map<int,int>& list);
};

//! Contains the global state of our chat application.
struct GlobalState {
  public:
    //! List of all messages sent by everyone.
    set<Message*> messageStore;

    //! The node representing our current chat process.
    Node node;

    //! A map from process ID to an ExternalNode pointer.
    map<int, ExternalNode*> externalNodes;

    GlobalState(int ownId, int* memberIds, int memberCount);
    ~GlobalState();

    //! Attempts to retrieve a message from the global message store.
    //! 
    //! @param fromId The sender process ID of our requested message.
    //! @param sequenceNumber The sequence number of our requested message.
    //! @return Either the message requested or NULL if it cannot be found.
    Message* getMessage(int fromId, int sequenceNumber);

    //! Stores a message into the global message store.
    //! 
    //! @param m A pointer to the message we want to store.
    void storeMessage(Message* m);
};

#endif
