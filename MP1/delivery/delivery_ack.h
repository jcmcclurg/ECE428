#ifndef DELIVERY_ACK_H
#define DELIVERY_ACK_H

#include <iostream>
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

    virtual ostream& dump(ostream& strm) const{
      strm << "DeliveryAcks{\n";
      for(map<int,int>::const_iterator it=deliveryAcks.begin(); it != deliveryAcks.end(); ++it){
        if(it->first != own_id){
          strm << "  "<< it->first << "  = " << it->second << "\n";
        }
        else{
          strm << " ["<< it->first << "] = " << it->second << "\n";
        }
      }
      strm << "}\n";
      return strm;
    }

    ostream& operator<<(ostream &strm, const DeliveryAcks* a){
      return a->dump(strm);
    }

    ostream& operator<<(ostream &strm, const DeliveryAcks& a){
      return a.dump(strm);
    }
};

#endif
