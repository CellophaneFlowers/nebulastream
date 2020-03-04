#include <Services/OptimizerService.hpp>
#include <Operators/OperatorJsonUtil.hpp>
#include <Optimizer/NESOptimizer.hpp>
#include <Topology/NESTopologyManager.hpp>
#include <Util/Logger.hpp>
#include <chrono>

using namespace NES;
using namespace web;
using namespace std;
using namespace std::chrono;

json::value OptimizerService::getExecutionPlanAsJson(InputQueryPtr inputQuery, string optimizationStrategyName) {
    return getExecutionPlan(inputQuery, optimizationStrategyName).first->getExecutionGraphAsJson();
}

pair<NESExecutionPlanPtr, long> OptimizerService::getExecutionPlan(InputQueryPtr inputQuery,
                                                                                       string optimizationStrategyName) {

    NESTopologyManager& nesTopologyManager = NESTopologyManager::getInstance();
    const NESTopologyPlanPtr& topologyPlan = nesTopologyManager.getNESTopologyPlan();
    NES_DEBUG("OptimizerService: topology=" << topologyPlan->getTopologyPlanString())

    NESOptimizer queryOptimizer;

    OperatorJsonUtil operatorJsonUtil;
    const json::value& basePlan = operatorJsonUtil.getBasePlan(inputQuery);

    NES_DEBUG("OptimizerService: query plan=" << basePlan)

    auto start = high_resolution_clock::now();

    const NESExecutionPlanPtr
        executionGraph = queryOptimizer.prepareExecutionGraph(optimizationStrategyName, inputQuery, topologyPlan);

    auto stop = high_resolution_clock::now();

    const auto duration = duration_cast<milliseconds>(stop - start);
    long durationInMillis = duration.count();

    return std::make_pair(executionGraph, durationInMillis);
}
