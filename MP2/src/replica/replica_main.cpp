#include "Replica.h"

#include "replica_handler.h"
#include "model/stringmachine.h"
#include "replicas.h"

using namespace std;
using namespace mp2;
using boost::shared_ptr;

int main(int argc, char **argv) {
	string pipedir;
	unsigned int myid;

	shared_ptr<Replicas> replicas(new Replicas(argc, argv, &myid));

	StringMachineFactory factory;
	shared_ptr<Replica> replica(new Replica(myid, factory, replicas));

	replicas->serve(replica, myid);

	return 0;
}
