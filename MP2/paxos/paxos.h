#ifndef __PAXOS_H__
#define __PAXOS_H__

#include <string>
#include <boost/shared_ptr.hpp>

#include "statemachine.h"
#include "replicas.h"

namespace mp2 {
  
class Agent {
  Proposer proposer_;
  Acceptor acceptor_;
  Learner learner_;
};

} // namespace

#endif

