#include "delivery_ack.h"

DeliveryAcks::DeliveryAcks(int own_id) : own_id(own_id){
  deliveryAcks[own_id] = 0;
}

void DeliveryAcks::deleteMember(int id){
  deliveryAcks.erase(id);
}

void DeliveryAcks::ack(int fromId , int sequenceNumber){
  deliveryAcks[fromId] = sequenceNumber;
}