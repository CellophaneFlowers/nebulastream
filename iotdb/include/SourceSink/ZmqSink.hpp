#ifndef ZMQSINK_HPP
#define ZMQSINK_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <zmq.hpp>

#include <SourceSink/DataSink.hpp>

namespace iotdb {

class ZmqSink : public DataSink {

 public:
  ZmqSink(const Schema& schema, const std::string& host, uint16_t port);
  ~ZmqSink() override;

  bool writeData(TupleBufferPtr input_buffer);
  void setup() override { connect(); };
  void shutdown() override{};
  const std::string toString() const override;
  int getPort();
    SinkType getType() const override;
  private:
  ZmqSink();

  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive& ar, const unsigned int version)
  {
    ar& boost::serialization::base_object<DataSink>(*this);
    ar& host;
    ar& port;
  }

  std::string host;
  uint16_t port;
  size_t tupleCnt;

  bool connected;
  zmq::context_t context;
  zmq::socket_t socket;

  bool connect();
  bool disconnect();
};
} // namespace iotdb

#endif // ZMQSINK_HPP