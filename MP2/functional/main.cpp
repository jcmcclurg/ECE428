#include <iostream>

#include "client/frontend.h"
#include "replica/replicas.h"
#include "settings.h"

using namespace mp2;
using namespace std;
using boost::shared_ptr;

int main(int argc, char **argv) {
	shared_ptr<Replicas> replicas(new Replicas(argc, argv));

	DEBUG((*replicas)[0].getLeader());

    sleep(1);

    DEBUG((*replicas)[1].getLeader());
	DEBUG((*replicas)[2].getLeader());

	(*replicas)[0].exit();

	sleep(1);

	DEBUG((*replicas)[1].getLeader());

	return 0;
}
