#ifndef __OPER_H_
#define __OPER_H_

#include <iostream>
#include <set>
#include <map>

using namespace std;

ostream& operator<<(ostream& strm, const map<int,int>& m);
ostream& operator<<(ostream& strm, const set<int>& m);

#endif
