/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        https://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <Runtime/NodeEngine.hpp>
#include <Runtime/TupleBuffer.hpp>
#include <Sinks/Mediums/FileSink.hpp>
#include <Sinks/Mediums/SinkMedium.hpp>
#include <Util/Logger/Logger.hpp>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include "Sinks/Formats/ArrowFormat.hpp"

namespace NES {

SinkMediumTypes FileSink::getSinkMediumType() { return SinkMediumTypes::FILE_SINK; }

FileSink::FileSink(SinkFormatPtr format,
                   Runtime::NodeEnginePtr nodeEngine,
                   uint32_t numOfProducers,
                   const std::string& filePath,
                   bool append,
                   QueryId queryId,
                   QuerySubPlanId querySubPlanId,
                   FaultToleranceType faultToleranceType,
                   uint64_t numberOfOrigins)
    : SinkMedium(std::move(format),
                 std::move(nodeEngine),
                 numOfProducers,
                 queryId,
                 querySubPlanId,
                 faultToleranceType,
                 numberOfOrigins,
                 std::make_unique<Windowing::MultiOriginWatermarkProcessor>(numberOfOrigins)) {
    this->filePath = filePath;
    this->append = append;
    if (!append) {
        if (std::filesystem::exists(filePath.c_str())) {
            bool success = std::filesystem::remove(filePath.c_str());
            NES_ASSERT2_FMT(success, "cannot remove file " << filePath.c_str());
        }
    }
    NES_DEBUG("FileSink: open file= {}", filePath);

    // only open the file stream if it is not an arrow file
    if(sinkFormat->getSinkFormat() != FormatTypes::ARROW_IPC_FORMAT) {
        if (!outputFile.is_open()) {
            outputFile.open(filePath, std::ofstream::binary | std::ofstream::app);
        }
        NES_ASSERT(outputFile.is_open(), "file is not open");
        NES_ASSERT(outputFile.good(), "file not good");
    }

    if (sinkFormat->getSinkFormat() == FormatTypes::ARROW_IPC_FORMAT) {
        // raise a warning if the file path does not have the streaming "arrows" extension some other system might
        // thus interpret the file differently with different extension
        // the MIME types for arrow files are ".arrow" for file format, and ".arrows" for streaming file format
        // see https://arrow.apache.org/faq/
        if(!(filePath.find(".arrows"))) {
            NES_WARNING("FileSink: An arrow ipc file without '.arrows' extension created as a file sink.");
        }
    }
}

FileSink::~FileSink() {
    NES_DEBUG("~FileSink: close file={}", filePath);
    outputFile.close();
}

std::string FileSink::toString() const {
    std::stringstream ss;
    ss << "FileSink(";
    ss << "SCHEMA(" << sinkFormat->getSchemaPtr()->toString() << ")";
    ss << ")";
    return ss.str();
}

void FileSink::setup() {}

void FileSink::shutdown() {}

bool FileSink::writeData(Runtime::TupleBuffer& inputBuffer, Runtime::WorkerContextRef) {
    // handle the case if we write to an arrow file
    if (sinkFormat->getSinkFormat() == FormatTypes::ARROW_IPC_FORMAT) {
        return writeDataToArrowFile(inputBuffer);
    }
    // otherwise call the regular function
    return writeDataToFile(inputBuffer);
}

std::string FileSink::getFilePath() const { return filePath; }

bool FileSink::writeDataToFile(Runtime::TupleBuffer &inputBuffer) {
    std::unique_lock lock(writeMutex);
    NES_TRACE("FileSink: getSchema medium {} format {} and mode {}",
              toString(),
              sinkFormat->toString(),
              this->getAppendAsString());

    if (!inputBuffer) {
        NES_ERROR("FileSink::writeDataToFile input buffer invalid");
        return false;
    }
    if (!schemaWritten) {
        NES_TRACE("FileSink::getData: write schema");
        auto schemaBuffer = sinkFormat->getSchema();
        if (schemaBuffer) {
            std::ofstream outputFile;
            if (sinkFormat->getSinkFormat() == FormatTypes::NES_FORMAT) {
                uint64_t idx = filePath.rfind('.');
                std::string shrinkedPath = filePath.substr(0, idx + 1);
                std::string schemaFile = shrinkedPath + "schema";
                NES_TRACE("FileSink::writeDataToFile: schema is ={} to file={}", sinkFormat->getSchemaPtr()->toString(), schemaFile);
                outputFile.open(schemaFile, std::ofstream::binary | std::ofstream::trunc);
            } else {
                outputFile.open(filePath, std::ofstream::binary | std::ofstream::trunc);
            }

            outputFile.write((char*) schemaBuffer->getBuffer(), schemaBuffer->getNumberOfTuples());
            outputFile.close();

            schemaWritten = true;
            NES_TRACE("FileSink::writeDataToFile: schema written");
        } else {
            NES_TRACE("FileSink::writeDataToFile: no schema written");
        }
    } else {
        NES_TRACE("FileSink::getData: schema already written");
    }

    NES_TRACE("FileSink::getData: write data to file= {}", filePath);
    auto dataBuffers = sinkFormat->getData(inputBuffer);

    for (auto& buffer : dataBuffers) {
        NES_TRACE("FileSink::getData: write buffer of size  {}", buffer.getNumberOfTuples());
        if (sinkFormat->getSinkFormat() == FormatTypes::NES_FORMAT) {
            outputFile.write((char*) buffer.getBuffer(),
                             buffer.getNumberOfTuples() * sinkFormat->getSchemaPtr()->getSchemaSizeInBytes());
        } else {
            outputFile.write((char*) buffer.getBuffer(), buffer.getNumberOfTuples());
        }
    }
    outputFile.flush();
    updateWatermarkCallback(inputBuffer);
    return true;
}

bool FileSink::writeDataToArrowFile(Runtime::TupleBuffer &inputBuffer) {
    std::unique_lock lock(writeMutex);

    // preliminary checks
    NES_TRACE("FileSink: getSchema medium {} format {} and mode {}",
              toString(),
              sinkFormat->toString(),
              this->getAppendAsString());

    if (!inputBuffer) {
        NES_ERROR("FileSink::writeDataToArrowFile input buffer invalid");
        return false;
    }

    // make arrow schema
    auto arrowFormat = std::dynamic_pointer_cast<ArrowFormat>(sinkFormat);
    std::shared_ptr<arrow::Schema> arrowSchema = arrowFormat->getArrowSchema();

    // open the arrow ipc file
    std::shared_ptr<arrow::io::FileOutputStream> outfileArrow;
    std::shared_ptr<arrow::ipc::RecordBatchWriter> arrowWriter;
    arrow::Status openStatus = openArrowFile(outfileArrow, arrowSchema, arrowWriter);

    if(!openStatus.ok()) {
        return false;
    }

    // get arrow arrays from tuple buffer
    std::vector<std::shared_ptr<arrow::Array>> arrowArrays = arrowFormat->getArrowArrays(inputBuffer);

    // make a record batch
    std::shared_ptr<arrow::RecordBatch> recordBatch = arrow::RecordBatch::Make(arrowSchema, arrowArrays[0]->length(), arrowArrays);

    // write the record batch
    auto write = arrowWriter->WriteRecordBatch(*recordBatch);

    // close the writer
    auto close = arrowWriter->Close();

    return true;
}

arrow::Status FileSink::openArrowFile(std::shared_ptr<arrow::io::FileOutputStream> arrowFileOutputStream,
                                      std::shared_ptr<arrow::Schema> arrowSchema,
                                      std::shared_ptr<arrow::ipc::RecordBatchWriter> arrowRecordBatchWriter) {
    // the macros initialize the arrowFileOutputStream and arrowRecordBatchWriter
    // if everything goes well return status OK
    // else the macros return failure
    ARROW_ASSIGN_OR_RAISE(arrowFileOutputStream, arrow::io::FileOutputStream::Open(filePath, append));
    ARROW_ASSIGN_OR_RAISE(arrowRecordBatchWriter, arrow::ipc::MakeStreamWriter(arrowFileOutputStream, arrowSchema));
    return arrow::Status::OK();
}

}// namespace NES
