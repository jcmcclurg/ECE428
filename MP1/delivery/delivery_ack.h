#ifndef DELIVERY_ACK_H
#define DELIVERY_ACK_H

#include <map>

using namespace std;

class DeliveryAcks{
  private:
    int own_id;
    map<int, int> deliveryAcks;

  public:
    DeliveryAcks(int own_id);
    void deleteMember(int id);
    void ack(int fromId, int sequenceNumber);
};

#endif
