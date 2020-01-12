#include <SourceSink/KafkaSource.hpp>

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

KafkaSource::KafkaSource() {}

KafkaSource::KafkaSource(const Schema &schema,
                         const std::string& brokers,
                         const std::string& topic,
                         const std::string& groupId,
                         const size_t kafkaConsumerTimeout) :
  DataSource(schema),
  topic(topic),
  kafkaConsumerTimeout(std::move(std::chrono::milliseconds(kafkaConsumerTimeout))),
  autoCommit(true) {

  config = {
    { "metadata.broker.list", brokers.c_str() },
    { "group.id", groupId },
    { "enable.auto.commit", true },
    { "auto.offset.reset", "latest" }
  };

  _connect();

  IOTDB_INFO("KAFKASOURCE " << this << ": Init KAFKA SOURCE to brokers " << brokers
             << ", topic " << topic)
}

KafkaSource::KafkaSource(const Schema& schema,
                         const std::string& topic,
                         const cppkafka::Configuration& config,
                         const size_t kafkaConsumerTimeout) :

  DataSource(schema),
  topic(topic),
  config(config),
  kafkaConsumerTimeout(std::move(std::chrono::milliseconds(kafkaConsumerTimeout))) {

  brokers = config.get("metadata.broker.list");
  groupId = config.get("group.id");
  std::istringstream(config.get("enable.auto.commit")) >> std::boolalpha >> autoCommit;

  _connect();
  IOTDB_INFO("KAFKASOURCE " << this << ": Init KAFKA SOURCE with config")
}

KafkaSource::~KafkaSource() { }

TupleBufferPtr KafkaSource::receiveData() {
  IOTDB_DEBUG("KAFKASOURCE tries to receive data...")

  cppkafka::Message msg = consumer->poll(kafkaConsumerTimeout);

  if (msg) {
    if (msg.get_error()) {
      if (! msg.is_eof()) {
        IOTDB_WARNING("KAFKASOURCE received error notification: " << msg.get_error())
          }
      return nullptr;
      } else {
      TupleBufferPtr buffer = BufferManager::instance().getBuffer();

      const size_t tupleSize = schema.getSchemaSize();
      const size_t tupleCnt = msg.get_payload().get_size() / tupleSize;

      IOTDB_DEBUG("KAFKASOURCE recv #tups: " << tupleCnt << ", tupleSize: " << tupleSize << ", msg: " << msg.get_payload())

      std::memcpy(buffer->getBuffer(), msg.get_payload().get_data(), buffer->getBufferSizeInBytes());
      buffer->setNumberOfTuples(tupleCnt);
      buffer->setTupleSizeInBytes(tupleSize);

      // XXX: maybe commit message every N times
      if (! autoCommit)
        consumer->commit(msg);
      return buffer;
    }
  }

  return nullptr;
}

const std::string KafkaSource::toString() const {
  std::stringstream ss;
  ss << "KAFKA_SOURCE(";
  ss << "SCHEMA(" << schema.toString() << "), ";
  ss << "BROKER(" << brokers << "), ";
  ss << "TOPIC(" << topic << "). ";
  return ss.str();
}

void KafkaSource::_connect() {
  consumer = std::make_unique<cppkafka::Consumer>(config);
  // set the assignment callback
  consumer->set_assignment_callback([&](cppkafka::TopicPartitionList& topicPartitions) {
                                      for (auto&& tpp : topicPartitions) {
                                        if (tpp.get_offset() == cppkafka::TopicPartition::Offset::OFFSET_INVALID) {
                                          IOTDB_WARNING("topic " << tpp.get_topic() << ", partition " << tpp.get_partition() << " get invalid offset " << tpp.get_offset())
                                          // TODO: enforce to set offset to -1 or abort ?
                                          auto tuple = consumer->query_offsets(tpp);
                                          size_t high = std::get<1>(tuple);
                                          tpp.set_offset(high-1);
                                        }
                                      }
                                      IOTDB_DEBUG("Got assigned " << topicPartitions.size() << " partitions")
  });
  // set the revocation callback
  consumer->set_revocation_callback([&](const cppkafka::TopicPartitionList& topicPartitions) {
                                      IOTDB_DEBUG(topicPartitions.size() << " partitions revoked")
  });

  consumer->subscribe({ topic });
}

SourceType KafkaSource::getType() const {
    return KAFKA_SOURCE;
}

}  // namespace
BOOST_CLASS_EXPORT(NES::KafkaSource);
