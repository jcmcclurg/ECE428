#include "../operators.h"
#include "timestamp.h"
#include <iostream>

Timestamp::Timestamp(int id, int* memberIds, int memberCount){
  ownId = id;
  for (int i = 0; i < memberCount; ++i) {
    timestamp[memberIds[i]] = 0;
  }
}

Timestamp::Timestamp(int id, set<int>& ids){
  ownId = id;
  for (set<int>::iterator it=ids.begin(); it != ids.end(); ++it){
    timestamp[*it] = 0;
  }
}

Timestamp::Timestamp(int id, map<int,int>& timestampMap){
  ownId = id;
  timestamp = timestampMap;
}
ostream& operator<<(ostream& strm, const Timestamp& m){
  strm << "Timestamp{"<< m.timestamp <<"}";
  return strm;
}
void Timestamp::step(){
  timestamp[ownId]++;
}

CausalityRelation Timestamp::compare(Timestamp& t) const {
  int numLess = 0;
  int numGreater = 0;
  
  for (map<int, int>::const_iterator it = timestamp.begin(); it != timestamp.end(); ++it) {
    if (it->second < t.timestamp[it->first]){ 
      numLess++;
      if (numGreater > 0) {
        return CONCURRENT;
      }
    }
    else if (it->second > t.timestamp[it->first]) {
      numGreater++;
      if (numLess > 0) {
        return CONCURRENT;
      }
    }
  }

  if (numLess > 0) { 
    return BEFORE;
  } else if (numGreater > 0) {
    return AFTER;
  } else {
    return CONCURRENT;
  }
}

map<int,int>& Timestamp::update(Timestamp& t){
  #ifdef DEBUG
  cout << "Updating timestamp[" << ownId << "]{";
  #endif
  for (map<int, int>::iterator it=timestamp.begin(); it != timestamp.end(); ++it){
    diff[it->first] = t.timestamp[it->first] - it->second;
    if(it->first != ownId && it->second < t.timestamp[it->first]){
      timestamp[it->first] = t.timestamp[it->first];
      #ifdef DEBUG
      cout << it->first << ":" << it->second << "->" << t.timestamp[it->first] << " ";
      #endif
    }
    #ifdef DEBUG
    else{
      cout << it->first << "=" << it->second << " ";
    }
    #endif
  }
  #ifdef DEBUG
  cout << "}" << endl;
  #endif
  return diff;
}
