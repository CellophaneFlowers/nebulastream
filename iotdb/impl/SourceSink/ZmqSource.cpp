#include <SourceSink/ZmqSource.hpp>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <zmq.hpp>

#include <NodeEngine/BufferManager.hpp>
#include <NodeEngine/Dispatcher.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <Util/Logger.hpp>
namespace NES {

ZmqSource::ZmqSource()
    : host(""),
      port(0),
      connected(false),
      context(zmq::context_t(1)),
      socket(zmq::socket_t(context, ZMQ_PULL)) {
  //This constructor is needed for Serialization
}

ZmqSource::ZmqSource(const Schema &schema, const std::string &host,
                     const uint16_t port)
    : DataSource(schema),
      host(host),
      port(port),
      connected(false),
      context(zmq::context_t(1)),
      socket(zmq::socket_t(context, ZMQ_PULL)) {
  IOTDB_DEBUG(
      "ZMQSOURCE  " << this << ": Init ZMQ ZMQSOURCE to " << host << ":" << port << "/")
}

ZmqSource::~ZmqSource() {
  bool success = disconnect();
  if (success) {
    IOTDB_DEBUG("ZMQSOURCE  " << this << ": Destroy ZMQ Source")
  } else {
    IOTDB_ERROR(
        "ZMQSOURCE  " << this << ": Destroy ZMQ Source failed cause it could not be disconnected")
    assert(0);
  }
  IOTDB_DEBUG("ZMQSOURCE  " << this << ": Destroy ZMQ Source")
}

TupleBufferPtr ZmqSource::receiveData() {
  IOTDB_DEBUG("ZMQSource  " << this << ": receiveData ")
  if (connect()) {
    try {
      // Receive new chunk of data
      zmq::message_t new_data;
      socket.recv(&new_data); // envelope - not needed at the moment
      size_t tupleCnt = *((size_t *) new_data.data());
      IOTDB_DEBUG("ZMQSource received #tups " << tupleCnt)

      zmq::message_t new_data2;
      socket.recv(&new_data2); // actual data

      // Get some information about received data
      size_t tuple_size = schema.getSchemaSize();
      // Create new TupleBuffer and copy data
      TupleBufferPtr buffer = BufferManager::instance().getBuffer();
      IOTDB_DEBUG("ZMQSource  " << this << ": got buffer ")

      // TODO: If possible only copy the content not the empty part
      std::memcpy(buffer->getBuffer(), new_data2.data(), buffer->getBufferSizeInBytes());
      buffer->setNumberOfTuples(tupleCnt);
      buffer->setTupleSizeInBytes(tuple_size);

      if (buffer->getBufferSizeInBytes() == new_data2.size()) {
        IOTDB_WARNING("ZMQSource  " << this << ": return buffer ")
      }

      return buffer;
    }
    catch (const zmq::error_t &ex) {
      // recv() throws ETERM when the zmq context is destroyed,
      //  as when AsyncZmqListener::Stop() is called
      if (ex.num() != ETERM) {
        IOTDB_ERROR("ZMQSOURCE: " << ex.what())
      }
    }
  } else {
    IOTDB_ERROR("ZMQSOURCE: Not connected!")
  }
  return nullptr;
}

const std::string ZmqSource::toString() const {
  std::stringstream ss;
  ss << "ZMQ_SOURCE(";
  ss << "SCHEMA(" << schema.toString() << "), ";
  ss << "HOST=" << host << ", ";
  ss << "PORT=" << port << ", ";
  return ss.str();
}

bool ZmqSource::connect() {
  if (!connected) {
    auto address = std::string("tcp://") + host + std::string(":") + std::to_string(port);

    try {
      socket.setsockopt(ZMQ_LINGER, 0);
      socket.bind(address.c_str());
      connected = true;
    }
    catch (const zmq::error_t &ex) {
      // recv() throws ETERM when the zmq context is destroyed,
      //  as when AsyncZmqListener::Stop() is called
      if (ex.num() != ETERM) {
        IOTDB_ERROR("ZMQSOURCE: " << ex.what())
      }
      connected = false;
    }
  }
  if (connected) {
    IOTDB_DEBUG("ZMQSOURCE  " << this << ": connected")
  } else {
    IOTDB_DEBUG("Exception: ZMQSOURCE  " << this << ": NOT connected")
  }
  return connected;
}

bool ZmqSource::disconnect() {
  if (connected) {
    socket.close();
    connected = false;
  }
  if (!connected) {
    IOTDB_DEBUG("ZMQSOURCE  " << this << ": disconnected")
  } else {
    IOTDB_DEBUG("ZMQSOURCE  " << this << ": NOT disconnected")
  }
  return !connected;
}
SourceType ZmqSource::getType() const {
    return ZMQ_SOURCE;
}

}  // namespace NES

BOOST_CLASS_EXPORT(NES::ZmqSource);
