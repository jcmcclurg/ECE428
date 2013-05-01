#include <iostream>

#include "frontend.h"
#include "replica/replicas.h"
#include "settings.h"
#include <iostream>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>

using namespace mp2;
using namespace std;
using boost::shared_ptr;

int main(int argc, char **argv) {
	shared_ptr<Replicas> replicas(new Replicas(argc, argv));

	FrontEnd frontEnd0(replicas, 0);
//	FrontEnd frontEnd1(replicas, 1);

	try {
		shared_ptr<StateMachine> machine0 = frontEnd0.get("s0");

		DEBUG("\nReads and writes succeed after 1 or 2 failures.");
		DEBUG("read s0:" << machine0->getState());
		DEBUG("write s0:" << machine0->apply("3"));
	(*replicas)[0].exit();
	(*replicas)[1].exit();
	(*replicas)[2].exit();
	} catch (ReplicaError e) {
			cerr << e.message << endl;
			return 1;
	}

	return 0;
}
