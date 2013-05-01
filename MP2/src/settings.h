#ifndef SETTINGS_H
#define SETTINGS_H

#include <iostream>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>

using namespace std;
#if DEBUGLEVEL == 1
	#define DEBUG(x) cout << x << endl; boost::this_thread::sleep(boost::posix_time::milliseconds(10))
#else
	#define DEBUG(x)
#endif

#define MIN_REPLICAS 3

#endif
