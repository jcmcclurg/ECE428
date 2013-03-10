#ifndef NODE_STATE_H_
#define NODE_STATE_H_

#include "../delivery/delivery_ack.h"
#include "../message/message.h"
#include "../timestamp/timestamp.h"

#include <set>

class NodeState{
  private:
    //! Unique identifier for this process. 
    int id;
        
    //! Sequence number of the last message delivered from this node.
    int sequenceNumber;

    //! List of all messages sent by this node (that might not have been delivered yet). Indexed by sequence number.
    map<int, Message> messageStore;

    //! Set keeping track of all nodes this process believes to have failed.
    set<int> failedNodes;

  public:
    //! Vector timestamp, with included IDs for easy access.
    Timestamp* timestamp;

    NodeState(int id) : id(id){ 
      timestamp = new Timestamp(id);
    }

    ~NodeState() {
      delete timestamp;
    }
};

class ExternalNodeState{
  private:
    //! Unique identifier for this external process. 
    int id;

    //! Sequence number of the last message delivered from this node.
    int latestDeliveredSequenceNumber;

    //! List of all messages sent by this node.
    map<int, Message> holdbackQueue;

  public:
    ExternalNodeState(int id) : id(id){}
};

struct GlobalState {
  NodeState state;
  map<int, ExternalNodeState> externalStates;
};

#endif