#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "../timestamp/timestamp.h"
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace std;

//! An enum representing a type of Message.
enum MessageType {HEARTBEAT, RETRANSREQUEST, MESSAGE};

//! A message that can be sent between chat clients.
class Message{
  friend bool operator<(const Message& a, const Message& b);
  friend ostream& operator<<(ostream& strm, const Message& m);
  
  private:
    //! Process specific properties
    int senderId;
    int sequenceNumber;
    bool needsDelete;
    Timestamp* timestamp;

    //! Message specific properties
    MessageType type;
    string message;

    //! Acknowledgements
    map<int, int> acknowledgements;

    //! Indicates failed nodes
    set<int> failedNodes;

  public:   
    Message(
        int senderId, 
        int sequenceNumber, 
        Timestamp& timestamp,
        MessageType type, 
        string message, 
        map<int, int>& acknowledgements,
        set<int>& failedNodes);

    //! Constructs a message from an encoded string. Since this message
    //! is external, it will need to delete the timestamp itself.
    Message(const char* encoded, int len);
    ~Message(){ if(needsDelete) delete timestamp; }

    int getSenderId() { return senderId; }
    int getSequenceNumber() { return sequenceNumber; }
    Timestamp& getTimestamp() { return *timestamp; }
    MessageType getType() { return type; }
    string getMessage() { return message; }
    map<int, int>& getAcknowledgements() { return acknowledgements; }
    set<int>& getFailedNodes() { return failedNodes; }
    
    //! Serializes a message into an encoded character array.
    //!
    //! @param encodedMessage A character buffer to store our serialized representation.
    //! @return The size of our serialized representation in bytes.
    int getEncodedMessage(char* encodedMessage);
};

bool operator<(const Message& a, const Message& b);

#endif
