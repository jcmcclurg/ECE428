# GTEST_DIR=lib/gtest-1.6.0
# THRIFT_DIR=/usr/local
GTEST_DIR=/class/ece428/libs/gtest-1.6.0
THRIFT_DIR=/class/ece428/libs
THRIFT=$(THRIFT_DIR)/bin/thrift
CFLAGS=-I./gen-cpp -g 
CXXFLAGS=-DDEBUGLEVEL=1 -I./src -I./gen-cpp -I$(GTEST_DIR)/include -I$(THRIFT_DIR)/include/thrift \
	-I$(THRIFT_DIR)/include -Wall -Wextra -g -DHAVE_NETINET_IN_H -DHAVE_INTTYPES_H

TESTSOURCES:=$(wildcard test/*.cpp)
TESTOBJS:=$(patsubst %.cpp,%.o,$(TESTSOURCES))
TESTS:=$(patsubst test/%,%,$(patsubst %.cpp,%,$(TESTSOURCES)))

FUNCSOURCES:=$(wildcard functional/*.cpp)
FUNCOBJS:=$(patsubst %.cpp,%.o,$(FUNCSOURCES))

REPLICAOBJS=src/replica/replica_handler.o src/replica/replicas.o

RPCSOURCES=gen-cpp/replica_constants.cpp gen-cpp/replica_types.cpp gen-cpp/Replica.cpp
RPCHEADERS:=$(patsubst %.cpp,%.h,$(RPCSOURCES))
RPCOBJS:=$(patsubst %.cpp,%.o,$(RPCSOURCES))

LDFLAGS=-L$(THRIFT_DIR)/lib -lpthread -lthrift -lboost_program_options-mt -lboost_filesystem-mt -lboost_system-mt -lboost_thread-mt

.PHONY: all test clean
all: replica client client2 client3

replica: $(REPLICAOBJS) src/replica/replica_main.o $(RPCOBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o bin/$@
 
client: src/client/client.o src/client/frontend.o src/replica/replicas.o $(RPCOBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o bin/$@
client2: src/client/client2.o src/client/frontend.o src/replica/replicas.o $(RPCOBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o bin/$@
client3: src/client/client3.o src/client/frontend.o src/replica/replicas.o $(RPCOBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o bin/$@
 
replicas_unittest: test/replicas_unittest.o $(REPLICAOBJS) $(RPCOBJS) gtest_main.a
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o bin/$@

stringmachine_unittest: test/stringmachine_unittest.o gtest_main.a
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o bin/$@

bankbalance_unittest: test/bankbalance_unittest.o gtest_main.a
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o bin/$@

functional: $(FUNCOBJS) $(REPLICAOBJS) $(RPCOBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o bin/$@

tests: $(TESTS)
	@for test in $^; do \
		./bin/$$test ; \
	done


.PHONY: thrift
thrift: 
	$(THRIFT) -gen cpp src/replica/replica.thrift

$(RPCSOURCES) $(RPCHEADERS): src/replica/replica.thrift
	$(THRIFT) -gen cpp $^

# Builds gtest.a and gtest_main.a.

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o: $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o: $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c $(GTEST_DIR)/src/gtest_main.cc

gtest.a: gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a: gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

clean:
	-rm .pipes/*
	-rm bin/*
	find . -name "*.o" | xargs rm

.PHONY: depend
depend: .depend

.depend: $(SRCS)
	-rm -f .depend
	-$(CXX) $(CXXFLAGS) -MM $^ > .depend

include .depend

