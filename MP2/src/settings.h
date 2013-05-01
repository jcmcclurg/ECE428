#ifndef SETTINGS_H
#define SETTINGS_H

#include <iostream>
#include <boost/format.hpp>

using namespace std;

#if DEBUGLEVEL == 1
	#define DEBUG(x) cout << x << endl
#else
	#define DEBUG(x)
#endif

#define MIN_REPLICAS 3

#endif
