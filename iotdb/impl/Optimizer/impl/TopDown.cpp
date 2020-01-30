#include "Optimizer/impl/TopDown.hpp"
#include <Util/Logger.hpp>
using namespace NES;

NESExecutionPlanPtr TopDown::initializeExecutionPlan(
    InputQueryPtr inputQuery, NESTopologyPlanPtr nesTopologyPlanPtr) {

    const OperatorPtr sinkOperator = inputQuery->getRoot();

    //find the source Node
    string streamName = inputQuery->getSourceStream()->getName();
    const deque<NESTopologyEntryPtr>& sourceNodes = StreamCatalog::instance()
        .getSourceNodesForLogicalStream(streamName);

    if (sourceNodes.empty()) {
        NES_ERROR("Unable to find the source node to place the operator");
        throw std::runtime_error("No available source node found in the network to place the operator");
    }

    const NESTopologyGraphPtr nesTopologyGraphPtr = nesTopologyPlanPtr->getNESTopologyGraph();

    NESExecutionPlanPtr nesExecutionPlanPtr = std::make_shared<NESExecutionPlan>();

    NES_INFO("TopDown: Placing operators on the nes topology.");
    placeOperators(nesExecutionPlanPtr, sinkOperator, sourceNodes, nesTopologyGraphPtr);

    NES_INFO("TopDown: Adding forward operators.");
    addForwardOperators(sourceNodes, nesTopologyGraphPtr, nesExecutionPlanPtr);

    NES_INFO("TopDown: Removing non resident operators from the execution nodes.");
    removeNonResidentOperators(nesExecutionPlanPtr);

    NES_INFO("TopDown: Generating complete execution Graph.");
    completeExecutionGraphWithNESTopology(nesExecutionPlanPtr, nesTopologyPlanPtr);

    //FIXME: We are assuming that throughout the pipeline the schema would not change.
    Schema schema= inputQuery->getSourceStream()->getSchema();
    addSystemGeneratedSourceSinkOperators(schema, nesExecutionPlanPtr);

    return nesExecutionPlanPtr;
}

void TopDown::placeOperators(NESExecutionPlanPtr executionPlanPtr, OperatorPtr sinkOperator,
                             deque<NESTopologyEntryPtr> nesSourceNodes, NESTopologyGraphPtr nesTopologyGraphPtr) {

    for (NESTopologyEntryPtr nesSourceNode : nesSourceNodes) {

        deque<OperatorPtr> operatorsToProcess = {sinkOperator};

        // Find the nodes where we can place the operators. First node will be sink and last one will be the target
        // source.
        deque<NESTopologyEntryPtr> candidateNodes = getCandidateNESNodes(nesTopologyGraphPtr, nesSourceNode);

        if (candidateNodes.empty()) {
            throw std::runtime_error("No path exists between sink and source");
        }

        // Loop till all operators are not placed.
        while (!operatorsToProcess.empty()) {
            OperatorPtr targetOperator = operatorsToProcess.front();
            operatorsToProcess.pop_front();

            if (targetOperator->getOperatorType() != OperatorType::SOURCE_OP) {

                string newOperatorName = "(OP-" + std::to_string(targetOperator->getOperatorId()) + ")";

                for (NESTopologyEntryPtr node : candidateNodes) {

                    if (executionPlanPtr->hasVertex(node->getId())) {

                        const ExecutionNodePtr existingExecutionNode = executionPlanPtr
                            ->getExecutionNode(node->getId());

                        size_t operatorId = targetOperator->getOperatorId();

                        vector<size_t>& residentOperatorIds = existingExecutionNode->getChildOperatorIds();
                        const auto exists = std::find(residentOperatorIds.begin(), residentOperatorIds.end(), operatorId);

                        if (exists != residentOperatorIds.end()) {

                            //Skip placement of the operator as already placed.
                            //Add child operators for placement
                            vector<OperatorPtr> nextOperatorsToProcess = targetOperator->getChildren();
                            copy(nextOperatorsToProcess.begin(), nextOperatorsToProcess.end(),
                                 back_inserter(operatorsToProcess));
                            break;
                        }
                    }

                    if (node->getRemainingCpuCapacity() > 0) {

                        if (executionPlanPtr->hasVertex(node->getId())) {

                            const ExecutionNodePtr executionNode = executionPlanPtr->getExecutionNode(node->getId());
                            addOperatorToExistingNode(targetOperator, executionNode);
                        } else {
                            createNewExecutionNode(executionPlanPtr, targetOperator, node);
                        }

                        // Add child operators for placement
                        vector<OperatorPtr> nextOperatorsToProcess = targetOperator->getChildren();
                        copy(nextOperatorsToProcess.begin(), nextOperatorsToProcess.end(),
                             back_inserter(operatorsToProcess));
                        break;
                    }
                }

                if (operatorsToProcess.empty()) {
                    throw std::runtime_error("Unable to schedule operator on the node");
                }
            } else {
                // if operator is of source type then find the sensor node and schedule it there directly.

                if (nesSourceNode->getRemainingCpuCapacity() <= 0) {
                    throw std::runtime_error("Unable to schedule source operator" + targetOperator->toString());
                }

                if (executionPlanPtr->hasVertex(nesSourceNode->getId())) {

                    const ExecutionNodePtr
                        executionNode = executionPlanPtr->getExecutionNode(nesSourceNode->getId());
                    addOperatorToExistingNode(targetOperator, executionNode);
                } else {
                    createNewExecutionNode(executionPlanPtr, targetOperator, nesSourceNode);
                }
            }
        }
    }
}

void TopDown::createNewExecutionNode(NESExecutionPlanPtr executionPlanPtr, OperatorPtr operatorPtr,
                                     NESTopologyEntryPtr nesNode) const {

    stringstream operatorName;
    operatorName << operatorTypeToString[operatorPtr->getOperatorType()]
                 << "(OP-" << to_string(operatorPtr->getOperatorId()) << ")";
    const ExecutionNodePtr
        executionNode = executionPlanPtr->createExecutionNode(operatorName.str(), to_string(nesNode->getId()),
                                                              nesNode, operatorPtr->copy());
    executionNode->addChildOperatorId(operatorPtr->getOperatorId());
    executionNode->getNESNode()->reduceCpuCapacity(1);
}

void TopDown::addOperatorToExistingNode(OperatorPtr operatorPtr, ExecutionNodePtr executionNode) const {

    stringstream operatorName;
    operatorName << operatorTypeToString[operatorPtr->getOperatorType()] << "(OP-"
                 << to_string(operatorPtr->getOperatorId()) << ")" << "=>" << executionNode->getOperatorName();
    executionNode->setOperatorName(operatorName.str());
    executionNode->addChildOperatorId(operatorPtr->getOperatorId());
    executionNode->getNESNode()->reduceCpuCapacity(1);
}
