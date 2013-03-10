#ifndef NODE_STATE_H_
#define NODE_STATE_H_

#include "../delivery/delivery_ack.h"
#include "../message/message.h"
#include "../timestamp/timestamp.h"

class NodeState{
  private:
      //! Sequence number of the last message delivered from this node.
    int latestDeliveredSequenceNumber;

    //! List of all messages sent by this node.
    map<int, Message> undeliveredMessages;

  public:
    //! Vector timestamp, with included IDs for easy access.
    Timestamp* timestamp;

    NodeState(int id) { 
      timestamp = new Timestamp(id);
    }

    ~NodeState() {
      delete timestamp;
    }
};

#endif