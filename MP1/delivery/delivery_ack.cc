#include "delivery_ack.h"

DeliveryAcks::DeliveryAcks(int own_id) : own_id(own_id){
  deliveryAcks[own_id] = 0;
}

void DeliveryAcks::deleteMember(int id){
  deliveryAcks.erase(id);
}

void DeliveryAcks::ack(int fromId, int sequenceNumber){
  deliveryAcks[fromId] = sequenceNumber;
}

void DeliveryAcks::update(DeliveryAcks* t){
  if(own_id == t->own_id){
    for (map<int,int>::iterator it=deliveryAcks.begin(); it != deliveryAcks.end(); ++it){
      if(deliveryAcks[it->first] < t->deliveryAcks[it->first]){
        deliveryAcks[it->first] = t->deliveryAcks[it->first];
      }
    }
  }
}