#ifndef MESSAGE_H_
#define MESSAGE_H_

#define MESSAGE_HEADER '\x1f'
#define TIMESTAMP_HEADER '\x1d'

#include "../delivery/delivery_ack.h"
#include "../timestamp/timestamp.h"
#include "../state/node_state.hpp"
#include <set>
#include <string>
#include <vector>

using namespace std;

enum MessageType {HEARTBEAT, RETRANSREQUEST, MESSAGE};

class Message{
  private:
    GlobalState& globalState;
    MessageType type;
    string message;

    int sequenceNumber;

  public:   
    Message(string encodedMessage);
    Message(GlobalState& globalState, MessageType type, string message);

    string getMessage();
    string getEncodedMessage();
};

#endif
