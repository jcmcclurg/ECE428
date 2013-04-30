GTEST_DIR=lib/gtest-1.6.0
THRIFT_DIR=/usr/local
THRIFT=$(THRIFT_DIR)/bin/thrift
CFLAGS=-I./gen-cpp -g
CXXFLAGS=-DDEBUGLEVEL=1 -I./src -I./gen-cpp -I$(GTEST_DIR)/include -I$(THRIFT_DIR)/include/thrift -I$(THRIFT_DIR)/include -Wall -Wextra -g

REPLICA_SRCS:=$(wildcard src/replica/*.cpp)
CLIENT_SRCS:=$(wildcard src/client/*.cpp)
TEST_SRCS:=$(wildcard test/*.cpp)

RPC_SRCS:=$(gen-cpp/*.cpp)
RPCHEADERS:=$(patsubst %.cpp, %.h, $(RPC_SRCS))
RPCOBJS:=$(patsubst %.cpp, %.o, $(RPC_SRCS))

LDFLAGS=-L$(THRIFT_DIR)/lib -lpthread -lthrift -lboost_program_options-mt -lboost_filesystem-mt -lboost_system-mt

.PHONY: all test clean
all: replica client

replica: $(REPLICA_SRCS) $(RPCOBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

client: $(CLIENT_SRCS) $(RPCOBJS) replicas.o 
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

test: $(TEST_SRCS) gtest_main.a
	$(CXX) $(CXXFLAGS) $^ -lpthread -o bin/$@

.PHONY: thrift
thrift: 
	$(THRIFT) -gen cpp replica.thrift

$(RPCSOURCES) $(RPCHEADERS): replica.thrift
	echo aldkfjlad
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
	-rm -f *.o gen-cpp/*.o $(TESTS) replica client .pipes/*

.PHONY: depend
depend: .depend

.depend: $(SRCS)
	-rm -f .depend
	-$(CXX) $(CXXFLAGS) -MM $^ > .depend

include .depend

