#ifndef __FRONTEND_H__
#define __FRONTEND_H__

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include "model/statemachine.h"
#include "replica/replicas.h"

namespace mp2 {
	
class FrontEnd
{
private:
	int leader;
	bool findingLeader;
public:
	int id;
	boost::shared_ptr<Replicas> replicas;
	FrontEnd(boost::shared_ptr<Replicas> replicas, int i);
	~FrontEnd();

	// create a new StateMachine and return a stub
	boost::shared_ptr<StateMachine> create(const std::string &name, const std::string &initState = "");
	// get a stub for an already created state machine;
	// returns NULL if it hasn't been created
	boost::shared_ptr<StateMachine> get(const std::string &name);
	// delete an existing state machine. Note: any remaining stubs for 
	// the state machine must throw an exception when apply() or getState() 
	// are called
	void remove(const std::string &name);
	int findLeader(void);
};

} // namespace

#endif

