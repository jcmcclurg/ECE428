/**
 * Autogenerated by Thrift Compiler (0.9.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef replica_TYPES_H
#define replica_TYPES_H

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>



namespace mp2 {

struct ErrorType {
  enum type {
    NOT_FOUND = 0,
    ALREADY_EXISTS = 1,
    NOT_LEADER = 2
  };
};

extern const std::map<int, const char*> _ErrorType_VALUES_TO_NAMES;

typedef struct _ReplicaError__isset {
  _ReplicaError__isset() : type(false), name(false), message(false) {}
  bool type;
  bool name;
  bool message;
} _ReplicaError__isset;

class ReplicaError : public ::apache::thrift::TException {
 public:

  static const char* ascii_fingerprint; // = "38C252E94E93B69D04EB3A6EE2F9EDFB";
  static const uint8_t binary_fingerprint[16]; // = {0x38,0xC2,0x52,0xE9,0x4E,0x93,0xB6,0x9D,0x04,0xEB,0x3A,0x6E,0xE2,0xF9,0xED,0xFB};

  ReplicaError() : type((ErrorType::type)0), name(), message() {
  }

  virtual ~ReplicaError() throw() {}

  ErrorType::type type;
  std::string name;
  std::string message;

  _ReplicaError__isset __isset;

  void __set_type(const ErrorType::type val) {
    type = val;
  }

  void __set_name(const std::string& val) {
    name = val;
  }

  void __set_message(const std::string& val) {
    message = val;
  }

  bool operator == (const ReplicaError & rhs) const
  {
    if (!(type == rhs.type))
      return false;
    if (!(name == rhs.name))
      return false;
    if (!(message == rhs.message))
      return false;
    return true;
  }
  bool operator != (const ReplicaError &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const ReplicaError & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

void swap(ReplicaError &a, ReplicaError &b);

typedef struct _Promise__isset {
  _Promise__isset() : success(false), acceptedProposalNumber(false), acceptedProposalValue(false) {}
  bool success;
  bool acceptedProposalNumber;
  bool acceptedProposalValue;
} _Promise__isset;

class Promise {
 public:

  static const char* ascii_fingerprint; // = "5C4D84321B3CBB236930D75F16BF3C14";
  static const uint8_t binary_fingerprint[16]; // = {0x5C,0x4D,0x84,0x32,0x1B,0x3C,0xBB,0x23,0x69,0x30,0xD7,0x5F,0x16,0xBF,0x3C,0x14};

  Promise() : success(0), acceptedProposalNumber(0), acceptedProposalValue(0) {
  }

  virtual ~Promise() throw() {}

  bool success;
  int32_t acceptedProposalNumber;
  int32_t acceptedProposalValue;

  _Promise__isset __isset;

  void __set_success(const bool val) {
    success = val;
  }

  void __set_acceptedProposalNumber(const int32_t val) {
    acceptedProposalNumber = val;
  }

  void __set_acceptedProposalValue(const int32_t val) {
    acceptedProposalValue = val;
  }

  bool operator == (const Promise & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    if (!(acceptedProposalNumber == rhs.acceptedProposalNumber))
      return false;
    if (!(acceptedProposalValue == rhs.acceptedProposalValue))
      return false;
    return true;
  }
  bool operator != (const Promise &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Promise & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

void swap(Promise &a, Promise &b);

} // namespace

#endif
