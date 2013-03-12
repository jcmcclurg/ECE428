#include "operators.h"

ostream& operator<<(ostream& strm, const map<int,int>& m){
  strm << "map{";
  for (map<int,int>::const_iterator it=m.begin(); it!=m.end(); ++it){
    strm << it->first << "=" << it->second << ",";
  }
  strm << "}";
  return strm;
}

ostream& operator<<(ostream& strm, const set<int>& m){
  strm << "set{";
  for (set<int>::const_iterator it=m.begin(); it!=m.end(); ++it){
    strm << *it << ",";
  }
  strm << "}";
  return strm;
}
