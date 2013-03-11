#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "../timestamp/timestamp.h"
#include <set>
#include <string>
#include <vector>

using namespace std;

enum MessageType {HEARTBEAT, RETRANSREQUEST, MESSAGE};

class Message{
  private:
    // Process specific properties
    int senderId;
    int sequenceNumber;
    Timestamp* timestamp;

    // Message specific properties
    MessageType type;
    string message;

    // Acknowledgements
    vector<pair<int, int> > acknowledgements;

    // Not actually serialized. Used for message store bookkeeping.
    set<int> undeliveredNodes;

  public:   
    Message(
        int senderId, 
        int sequenceNumber, 
        Timestamp& timestamp,
        MessageType type, 
        string message, 
        vector<pair<int, int> > acknowledgements);
    Message(const string& encoded);
    ~Message();

    int getSenderId() { return senderId; }
    int getSequenceNumber() { return sequenceNumber; }
    Timestamp* getTimestamp() { return timestamp; }
    MessageType getType() { return type; }
    string getMessage() { return message; }
    vector<pair<int, int> >& getAcknowledgements() { return acknowledgements; }

    string getEncodedMessage();
    set<int>& getUndeliveredNodes() { return undeliveredNodes; }
};

#endif
