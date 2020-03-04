#ifndef INCLUDE_PIPELINESTAGE_H_
#define INCLUDE_PIPELINESTAGE_H_
#include <memory>
#include <vector>
#include <NodeEngine/TupleBuffer.hpp>

namespace NES {

class PipelineStage;
typedef std::shared_ptr<PipelineStage> PipelineStagePtr;

template<class PartialAggregateType>
class WindowSliceStore;

class WindowHandler;
typedef std::shared_ptr<WindowHandler> WindowHandlerPtr;

class ExecutablePipeline;
typedef std::shared_ptr<ExecutablePipeline> ExecutablePipelinePtr;

class QueryExecutionPlan;
typedef std::shared_ptr<QueryExecutionPlan> QueryExecutionPlanPtr;

class WindowManager;

class PipelineStage {
 public:
  PipelineStage(
      uint32_t pipelineStageId,
      QueryExecutionPlanPtr queryExecutionPlanPtr,
      ExecutablePipelinePtr executablePipeline,
      WindowHandlerPtr windowHandler);
  explicit PipelineStage(uint32_t pipelineStageId,
                         QueryExecutionPlanPtr queryExecutionPlanPtr, ExecutablePipelinePtr executablePipeline);
  bool execute(TupleBufferPtr inputBuffer,
               TupleBufferPtr outputBuffer);

  /**
 * @brief Initialises a pipeline stage
 */
  void setup();

  /**
   * @brief Starts a pipeline stage
   * @return boolean if successful
   */
  bool start();

  /**
   * @brief Stops pipeline stage
   * @return
   */
  bool stop();

  ~PipelineStage();
 private:
  uint32_t pipelineStageId;
  QueryExecutionPlanPtr queryExecutionPlanPtr;
  ExecutablePipelinePtr executablePipeline;
  WindowHandlerPtr windowHandler;
};
typedef std::shared_ptr<PipelineStage> PipelineStagePtr;

class CompiledCode;
typedef std::shared_ptr<CompiledCode> CompiledCodePtr;

PipelineStagePtr createPipelineStage(uint32_t pipelineStageId,
                                     const QueryExecutionPlanPtr &queryExecutionPlanPtr,const ExecutablePipelinePtr &compiled_code,
                                     const WindowHandlerPtr &window_handler);
PipelineStagePtr createPipelineStage(uint32_t pipelineStageId,
                                     const QueryExecutionPlanPtr &queryExecutionPlanPtr,const ExecutablePipelinePtr &compiled_code);

} // namespace NES

#endif /* INCLUDE_PIPELINESTAGE_H_ */
