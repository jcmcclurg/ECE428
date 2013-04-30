#ifndef PAXOS_H
#define PAXOS_H

#include <string>
#include <boost/shared_ptr.hpp>

#include "models/statemachine.h"
#include "replicas/replicas.h"

namespace mp2 {
  
class Agent {
  Proposer proposer_;
  Acceptor acceptor_;
  Learner learner_;
};

} // namespace

#endif

