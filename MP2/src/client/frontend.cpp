#include <time.h>
#include <boost/shared_ptr.hpp>

#include "frontend.h"
#include "model/statemachine.h"
#include "settings.h"


using namespace std;
using namespace mp2;
using boost::shared_ptr;

class StateMachineStub : public mp2::StateMachine {
private:
	FrontEnd* front;
	int leader;
	const string name;

public:
	StateMachineStub(FrontEnd* e, const string &name)
		: leader(e->findLeader()), name(name) {}

	virtual string apply(const string & operation) {
		string result;
		(*(front->replicas))[leader].apply(result, name, operation);
		return result;
	}

	virtual string getState(void) const {
		string result;
		(*(front->replicas))[leader].getState(result, name);
		return result;
	}
};

FrontEnd::FrontEnd(boost::shared_ptr<Replicas> eeplicas, int i) : replicas(replicas),  id(i), leader(-1) {
	DEBUG("FE " << id << " started up with access to " << (*replicas).numReplicas() << " replica managers.");
}

FrontEnd::~FrontEnd() { }

shared_ptr<StateMachine> FrontEnd::create(const string &name, const string &initialState) {
	if(leader == -1){
		findLeader();
	}
	DEBUG("FE " << id << " creating state machine " << name);
	(*replicas)[leader].create(name, initialState);

	return get(name);
}

shared_ptr<StateMachine> FrontEnd::get(const string &name) {
	if(leader == -1){
		findLeader();
	}
	shared_ptr<StateMachine> result(new StateMachineStub(this, name));
	return result;
}

int FrontEnd::findLeader(void){
	DEBUG("FE " << id << " trying to find leader...");
	leader = -1;
	string result;
	while(leader == -1){
		for(int i = 0; i < (int)(*replicas).numReplicas(); i++){
			try{
				(*replicas)[i].apply(result,"","leader?");
				leader = i;
				DEBUG("FE " << id << " found leader " << i);
				break;
			}
			catch(ReplicaError){
			}
		}
		sleep(1000);
	}

	return 0;
}

void FrontEnd::remove(const string &name) {
	if(leader == -1){
		findLeader();
	}
	(*replicas)[0].remove(name);
}


