#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "../timestamp/timestamp.h"
#include <set>
#include <string>
#include <vector>

using namespace std;

enum MessageType {HEARTBEAT, RETRANSREQUEST, MESSAGE};

class Message{
  friend bool operator<(const Message& a, const Message& b);
  private:
    // Process specific properties
    int senderId;
    int sequenceNumber;
    bool needsDelete;
    Timestamp* timestamp;

    // Message specific properties
    MessageType type;
    string message;

    // Acknowledgements
    map<int, int> acknowledgements;

    // Indicates failed nodes
    vector<int> failedNodes;

  public:   
    Message(
        int senderId, 
        int sequenceNumber, 
        Timestamp& timestamp,
        MessageType type, 
        string message, 
        map<int, int> acknowledgements,
        vector<int> failNodes);
    Message(const string& encoded);
    ~Message(){ if(needsDelete) delete timestamp; }

    int getSenderId() { return senderId; }
    int getSequenceNumber() { return sequenceNumber; }
    Timestamp& getTimestamp() { return timestamp; }
    MessageType getType() { return type; }
    string getMessage() { return message; }
    map<int, int>& getAcknowledgements() { return acknowledgements; }

    string getEncodedMessage();
    set<int>& getFailedNodes() { return undeliveredNodes; }
};
bool operator<(const Message& a, const Message& b);

#endif
