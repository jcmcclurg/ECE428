#include <iostream>

#include "frontend.h"
#include "replica/replicas.h"

using namespace mp2;
using namespace std;
using boost::shared_ptr;

int main(int argc, char **argv) {
	shared_ptr<Replicas> replicas(new Replicas(argc, argv));

	FrontEnd frontEnd(replicas, 1);

	try {
		shared_ptr<StateMachine> machine = frontEnd.create("statemachine1", "initialstate");

		cout << machine->getState() << endl;

		cout << machine->apply("newstate") << endl;

		cout << machine->getState() << endl;

		frontEnd.remove("statemachine1");

	} catch (ReplicaError e) {
			cerr << e.message << endl;
			return 1;
	}

	return 0;
}
