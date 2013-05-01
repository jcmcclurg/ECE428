#include <iostream>

#include "frontend.h"
#include "replica/replicas.h"
#include "settings.h"

using namespace mp2;
using namespace std;
using boost::shared_ptr;

int main(int argc, char **argv) {
	shared_ptr<Replicas> replicas(new Replicas(argc, argv));

	FrontEnd frontEnd0(replicas, 0);
	FrontEnd frontEnd1(replicas, 1);

	try {
		shared_ptr<StateMachine> machine0 = frontEnd0.create("s0", "0");
		shared_ptr<StateMachine> machine1 = frontEnd1.create("s1", "1");

		DEBUG("read s0:" << machine0->getState());
		cout << "read s1:" << machine1->getState() << endl;

		cout << "write s0:" << machine0->apply("new0") << endl;
		cout << "read s1:" << machine1->getState() << endl;
		(*replicas)[2].exit();

		cout << "write s1:" << machine0->apply("new1") << endl;
		cout << "read s0:" << machine0->getState() << endl;

		frontEnd0.remove("s0");

	} catch (ReplicaError e) {
			cerr << e.message << endl;
			return 1;
	}

	return 0;
}
