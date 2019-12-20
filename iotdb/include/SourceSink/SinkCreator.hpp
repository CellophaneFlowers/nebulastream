#ifndef INCLUDE_SOURCESINK_SINKCREATOR_HPP_
#define INCLUDE_SOURCESINK_SINKCREATOR_HPP_
#include <SourceSink/DataSink.hpp>

namespace iotdb {

/**
 * @brief create test sink
 * @Note this method is currently not implemented
 */
const DataSinkPtr createTestSink();

/**
 * @brief create a print test sink without a schema
 * @param output stream
 * @return a data sink pointer
 */
const DataSinkPtr createPrintSinkWithoutSchema(std::ostream& out);

/**
 * @brief create a print test sink with a schema
 * @param schema of sink
 * @param output stream
 * @return a data sink pointer
 */
const DataSinkPtr createPrintSinkWithSchema(const Schema& schema, std::ostream& out);

/**
 * @brief create a binary test sink without a schema
 * @param path to file
 * @return a data sink pointer
 */
const DataSinkPtr createBinaryFileSinkWithoutSchema(const std::string& filePath);

/**
 * @brief create a binary test sink with a schema
 * @param schema of sink
 * @param path to file
 * @return a data sink pointer
 */
const DataSinkPtr createBinaryFileSinkWithSchema(const Schema& schema,
                                                 const std::string& filePath);

/**
 * @brief create a ZMQ test sink with a schema
 * @param schema of sink
 * @param hostname as sting
 * @param port at uint16
 * @return a data sink pointer
 */
const DataSinkPtr createZmqSink(const Schema& schema, const std::string& host,
                                const uint16_t port);

/**
 * @brief create test sink of YSB benchmark
 */
const DataSinkPtr createYSBPrintSink();

}
#endif /* INCLUDE_SOURCESINK_SINKCREATOR_HPP_ */
