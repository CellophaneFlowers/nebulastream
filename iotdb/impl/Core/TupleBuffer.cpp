#include <bitset>
#include <NodeEngine/TupleBuffer.hpp>
#include <exception>
#include <boost/endian/buffers.hpp>  // see Synopsis below
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
BOOST_CLASS_EXPORT_IMPLEMENT (NES::TupleBuffer);
#include <cstring>
#include <iostream>
#include <sstream>
#include <Util/Logger.hpp>
using namespace std;

namespace NES {

TupleBuffer::TupleBuffer(void *_buffer, const size_t _buffer_size_bytes,
                         const uint32_t _tuple_size_bytes,
                         const uint32_t _num_tuples)
    :
    buffer(_buffer),
    bufferSizeInBytes(_buffer_size_bytes),
    tupleSizeInBytes(_tuple_size_bytes),
    numberOfTuples(_num_tuples),
    useCnt(0) {
}

void TupleBuffer::copyInto(const TupleBufferPtr other) {
  if (other && other.get() != this) {
    this->bufferSizeInBytes = other->bufferSizeInBytes;
    this->tupleSizeInBytes = other->tupleSizeInBytes;
    this->numberOfTuples = other->numberOfTuples;
    std::memcpy(this->buffer, other->buffer, other->bufferSizeInBytes);
  }
}

TupleBuffer& TupleBuffer::operator=(const TupleBuffer &other) {
  if (this != &other) {
    this->bufferSizeInBytes = other.bufferSizeInBytes;
    this->tupleSizeInBytes = other.tupleSizeInBytes;
    this->numberOfTuples = other.numberOfTuples;
    std::memcpy(this->buffer, other.buffer, other.bufferSizeInBytes);
  }
  return *this;
}

void TupleBuffer::print() {
  std::cout << "buffer address=" << buffer << std::endl;
  std::cout << "buffer size=" << bufferSizeInBytes << std::endl;
  std::cout << "buffer tuple_size_bytes=" << tupleSizeInBytes << std::endl;
  std::cout << "buffer num_tuples=" << numberOfTuples << std::endl;
}

size_t TupleBuffer::getNumberOfTuples() {
  return numberOfTuples;
}

void TupleBuffer::setNumberOfTuples(size_t number) {
  numberOfTuples = number;
}

void* TupleBuffer::getBuffer() {
  return buffer;
}

size_t TupleBuffer::getBufferSizeInBytes() {
  return bufferSizeInBytes;
}

void TupleBuffer::setBufferSizeInBytes(size_t size) {
  bufferSizeInBytes = size;
}

size_t TupleBuffer::getTupleSizeInBytes() {
  return tupleSizeInBytes;
}

void TupleBuffer::setTupleSizeInBytes(size_t size) {
  tupleSizeInBytes = size;
}

void TupleBuffer::setUseCnt(size_t size) {
  useCnt = size;
}

size_t TupleBuffer::getUseCnt() {
  return useCnt;
}

bool TupleBuffer::decrementUseCntAndTestForZero() {
  if (useCnt >= 1) {
    useCnt--;
  } else
    assert(0);

  return useCnt == 0;
}

bool TupleBuffer::incrementUseCnt() {
  //TODO: should this be thread save?
  useCnt++;
}

std::string TupleBuffer::printTupleBuffer(Schema schema) {
  std::stringstream ss;
  for (size_t i = 0; i < numberOfTuples; i++) {
    size_t offset = 0;
    for (size_t j = 0; j < schema.getSize(); j++) {
      auto field = schema[j];
      size_t fieldSize = field->getFieldSize();
      DataTypePtr ptr = field->getDataType();
      std::string str = ptr->convertRawToString(
          buffer + offset + i * schema.getSchemaSize());
      ss << str.c_str();
      if (j < schema.getSize() - 1) {
        ss << ",";
      }
      offset += fieldSize;
    }
    ss << std::endl;
  }
  return ss.str();
}

/** possible types are:
 *  INT8,UINT8,INT16,UINT16,INT32,UINT32,INT64,FLOAT32,UINT64,FLOAT64,BOOLEAN,CHAR,DATE,VOID_TYPE
 *  Code for debugging:
 *  std::bitset<16> x(*orgVal);
 *  std::cout << "16-BEFORE biseq=" << x << " orgVal=" << *orgVal << endl;
 *  u_int16_t val = boost::endian::endian_reverse(*orgVal);
 *  std::bitset<16> x2(val);
 *  std::cout << "16-After biseq=" << x2 << " val=" << val << endl;
 *
 *  cout << "buffer=" << buffer << " offset=" << offset << " i=" << i
 *            << " tupleSize=" << tupleSize << " fieldSize=" << fieldSize
 *            << " res=" << (char*)buffer + offset + i * tupleSize << endl;
 */
void TupleBuffer::revertEndianness(Schema schema) {
  size_t tupleSize = schema.getSchemaSize();
  for (size_t i = 0; i < numberOfTuples; i++) {
    size_t offset = 0;
    for (size_t j = 0; j < schema.getSize(); j++) {
      auto field = schema[j];
      size_t fieldSize = field->getFieldSize();

      if (field->getDataType()->toString() == "UINT8") {
        u_int8_t* orgVal = (u_int8_t*) buffer + offset + i * tupleSize;
        memcpy((char*) buffer + offset + i * tupleSize, orgVal, fieldSize);
      } else if (field->getDataType()->toString() == "UINT16") {
        u_int16_t* orgVal = (u_int16_t*) ((char*) buffer + offset
            + i * tupleSize);
        u_int16_t val = boost::endian::endian_reverse(*orgVal);
        memcpy((char*) buffer + offset + i * tupleSize, &val, fieldSize);
      } else if (field->getDataType()->toString() == "UINT32") {
        uint32_t* orgVal = (uint32_t*) ((char*) buffer + offset + i * tupleSize);
        uint32_t val = boost::endian::endian_reverse(*orgVal);
        memcpy((char*) buffer + offset + i * tupleSize, &val, fieldSize);
      } else if (field->getDataType()->toString() == "UINT64") {
        uint64_t* orgVal = (uint64_t*) ((char*) buffer + offset + i * tupleSize);
        uint64_t val = boost::endian::endian_reverse(*orgVal);
        memcpy((char*) buffer + offset + i * tupleSize, &val, fieldSize);
      } else if (field->getDataType()->toString() == "INT8") {
        int8_t* orgVal = (int8_t*) buffer + offset + i * tupleSize;
        int8_t val = boost::endian::endian_reverse(*orgVal);
        memcpy((char*) buffer + offset + i * tupleSize, &val, fieldSize);
      } else if (field->getDataType()->toString() == "INT16") {
        int16_t* orgVal = (int16_t*) ((char*) buffer + offset + i * tupleSize);
        int16_t val = boost::endian::endian_reverse(*orgVal);
        memcpy((char*) buffer + offset + i * tupleSize, &val, fieldSize);
      } else if (field->getDataType()->toString() == "INT32") {
        int32_t* orgVal = (int32_t*) ((char*) buffer + offset + i * tupleSize);
        int32_t val = boost::endian::endian_reverse(*orgVal);
        memcpy((char*) buffer + offset + i * tupleSize, &val, fieldSize);
      } else if (field->getDataType()->toString() == "INT64") {
        int64_t* orgVal = (int64_t*) ((char*) buffer + offset + i * tupleSize);
        int64_t val = boost::endian::endian_reverse(*orgVal);
        memcpy((char*) buffer + offset + i * tupleSize, &val, fieldSize);
      } else if (field->getDataType()->toString() == "FLOAT32") {
        uint32_t* orgVal = (uint32_t*) ((char*) buffer + offset + i * tupleSize);
        uint32_t val = boost::endian::endian_reverse(*orgVal);
        memcpy((char*) buffer + offset + i * tupleSize, &val, fieldSize);
      } else if (field->getDataType()->toString() == "FLOAT64") {
        uint64_t* orgVal = (uint64_t*) ((char*) buffer + offset + i * tupleSize);
        uint64_t val = boost::endian::endian_reverse(*orgVal);
        memcpy((char*) buffer + offset + i * tupleSize, &val, fieldSize);
      } else if (field->getDataType()->toString() == "CHAR") {
        //TODO: I am not sure if we have to convert char at all because it is one byte only
        throw new Exception(
            "Data type float is currently not supported for endian conversation");
      } else {
        throw new Exception(
            "Data type " + field->getDataType()->toString()
                + " is currently not supported for endian conversation");
      }
      offset += fieldSize;
    }
  }
}

}

// namespace NES

