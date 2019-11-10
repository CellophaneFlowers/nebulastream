#include <CodeGen/QueryExecutionPlan.hpp>
#include <Core/TupleBuffer.hpp>
#include <NodeEngine/BufferManager.hpp>
#include <NodeEngine/Task.hpp>
#include "../../include/SourceSink/DataSource.hpp"

namespace iotdb {

Task::Task(QueryExecutionPlanPtr _qep, uint32_t _pipeline_stage_id,
           const TupleBufferPtr pBuf)
    : qep(_qep),
      pipeline_stage_id(_pipeline_stage_id),
      buf(pBuf) {
}

bool Task::execute() {
  return qep->executeStage(pipeline_stage_id, buf);
}

void Task::releaseInputBuffer() {
  BufferManager::instance().releaseBuffer(buf);
}

size_t Task::getNumberOfTuples() {
    return buf->num_tuples;
  }

}  // namespace iotdb
