// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "Replica.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace  ::mp2;

class ReplicaHandler : virtual public ReplicaIf {
 public:
  ReplicaHandler() {
    // Your initialization goes here
  }

  void create(const std::string& name, const std::string& initialState) {
    // Your implementation goes here
    printf("create\n");
  }

  void apply(std::string& _return, const std::string& name, const std::string& operation) {
    // Your implementation goes here
    printf("apply\n");
  }

  void getState(std::string& _return, const int16_t client, const std::string& name) {
    // Your implementation goes here
    printf("getState\n");
  }

  void remove(const std::string& name) {
    // Your implementation goes here
    printf("remove\n");
  }

  int16_t prepareGetState(const int16_t client, const std::string& name) {
    // Your implementation goes here
    printf("prepareGetState\n");
  }

  int16_t getLeader() {
    // Your implementation goes here
    printf("getLeader\n");
  }

  int16_t getQueueLen() {
    // Your implementation goes here
    printf("getQueueLen\n");
  }

  int16_t getBwUtilization() {
    // Your implementation goes here
    printf("getBwUtilization\n");
  }

  int16_t getMemUtilization() {
    // Your implementation goes here
    printf("getMemUtilization\n");
  }

  int16_t startLeaderElection() {
    // Your implementation goes here
    printf("startLeaderElection\n");
  }

  bool stateExists(const std::string& name) {
    // Your implementation goes here
    printf("stateExists\n");
  }

  void notifyFinishedReading(const int16_t rmid, const int16_t client, const std::string& name) {
    // Your implementation goes here
    printf("notifyFinishedReading\n");
  }

  void prepare(Promise& _return, const int32_t n) {
    // Your implementation goes here
    printf("prepare\n");
  }

  bool accept(const int32_t n, const int32_t value) {
    // Your implementation goes here
    printf("accept\n");
  }

  void inform(const int32_t value) {
    // Your implementation goes here
    printf("inform\n");
  }

  void exit() {
    // Your implementation goes here
    printf("exit\n");
  }

};

int main(int argc, char **argv) {
  int port = 9090;
  shared_ptr<ReplicaHandler> handler(new ReplicaHandler());
  shared_ptr<TProcessor> processor(new ReplicaProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;
}

