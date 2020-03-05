#include <Optimizer/impl/TopDown.hpp>
#include <Operators/Operator.hpp>
#include <Util/Logger.hpp>
#include <Optimizer/utils/PathFinder.hpp>

namespace NES {

NESExecutionPlanPtr TopDown::initializeExecutionPlan(
    InputQueryPtr inputQuery, NESTopologyPlanPtr nesTopologyPlanPtr) {

    const OperatorPtr sinkOperator = inputQuery->getRoot();

    //find the source Node
    string streamName = inputQuery->getSourceStream()->getName();
    const vector<NESTopologyEntryPtr>& sourceNodes = StreamCatalog::instance()
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
    addForwardOperators(sourceNodes, nesTopologyGraphPtr->getRoot(), nesExecutionPlanPtr);

    NES_INFO("TopDown: Generating complete execution Graph.");
    completeExecutionGraphWithNESTopology(nesExecutionPlanPtr, nesTopologyPlanPtr);

    //FIXME: We are assuming that throughout the pipeline the schema would not change.
    Schema schema = inputQuery->getSourceStream()->getSchema();
    addSystemGeneratedSourceSinkOperators(schema, nesExecutionPlanPtr);

    return nesExecutionPlanPtr;
}

void TopDown::placeOperators(NESExecutionPlanPtr executionPlanPtr, OperatorPtr sinkOperator,
                             vector<NESTopologyEntryPtr> nesSourceNodes, NESTopologyGraphPtr nesTopologyGraphPtr) {

    PathFinder pathFinder;

    for (NESTopologyEntryPtr nesSourceNode : nesSourceNodes) {

        deque<OperatorPtr> operatorsToProcess = {sinkOperator};

        // Find the nodes where we can place the operators. First node will be sink and last one will be the target
        // source.
        std::vector<NESTopologyEntryPtr>
            candidateNodes = pathFinder.findPathBetween(nesSourceNode, nesTopologyGraphPtr->getRoot());

        if (candidateNodes.empty()) {
            throw std::runtime_error("No path exists between sink and source");
        }

        // Loop till all operators are not placed.
        while (!operatorsToProcess.empty()) {
            OperatorPtr targetOperator = operatorsToProcess.front();
            operatorsToProcess.pop_front();

            if (targetOperator->getOperatorType() != OperatorType::SOURCE_OP) {

                string newOperatorName = "(OP-" + std::to_string(targetOperator->getOperatorId()) + ")";

                for (auto node = candidateNodes.rbegin(); node != candidateNodes.rend(); node++) {

                    if (executionPlanPtr->hasVertex(node.operator*()->getId())) {

                        const ExecutionNodePtr existingExecutionNode = executionPlanPtr
                            ->getExecutionNode(node.operator*()->getId());

                        size_t operatorId = targetOperator->getOperatorId();

                        vector<size_t>& residentOperatorIds = existingExecutionNode->getChildOperatorIds();
                        const auto
                            exists = std::find(residentOperatorIds.begin(), residentOperatorIds.end(), operatorId);

                        if (exists != residentOperatorIds.end()) {

                            //Skip placement of the operator as already placed.
                            //Add child operators for placement
                            vector<OperatorPtr> nextOperatorsToProcess = targetOperator->getChildren();
                            copy(nextOperatorsToProcess.begin(), nextOperatorsToProcess.end(),
                                 back_inserter(operatorsToProcess));
                            break;
                        }
                    }

                    if (node.operator*()->getRemainingCpuCapacity() > 0) {

                        if (executionPlanPtr->hasVertex(node.operator*()->getId())) {

                            const ExecutionNodePtr
                                executionNode = executionPlanPtr->getExecutionNode(node.operator*()->getId());
                            addOperatorToExistingNode(targetOperator, executionNode);
                        } else {
                            createNewExecutionNode(executionPlanPtr, targetOperator, node.operator*());
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
                nesSourceNode->reduceCpuCapacity(1);
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
    executionNode->addOperatorId(operatorPtr->getOperatorId());
}

void TopDown::addOperatorToExistingNode(OperatorPtr operatorPtr, ExecutionNodePtr executionNode) const {

    stringstream operatorName;
    operatorName << operatorTypeToString[operatorPtr->getOperatorType()] << "(OP-"
                 << to_string(operatorPtr->getOperatorId()) << ")" << "=>" << executionNode->getOperatorName();
    executionNode->setOperatorName(operatorName.str());
    executionNode->addChild(operatorPtr->copy());
    executionNode->addOperatorId(operatorPtr->getOperatorId());
}

void TopDown::addForwardOperators(vector<NESTopologyEntryPtr> sourceNodes, NESTopologyEntryPtr rootNode,
                                  NESExecutionPlanPtr nesExecutionPlanPtr) const {

    PathFinder pathFinder;

    for (NESTopologyEntryPtr targetSource: sourceNodes) {

        //Find the list of nodes connecting the source and destination nodes
        std::vector<NESTopologyEntryPtr> candidateNodes = pathFinder.findPathBetween(targetSource, rootNode);

        for (NESTopologyEntryPtr candidateNode: candidateNodes) {

            if (candidateNode->getCpuCapacity() == candidateNode->getRemainingCpuCapacity()) {
                nesExecutionPlanPtr->createExecutionNode("FWD", to_string(candidateNode->getId()), candidateNode,
                    /**executableOperator**/nullptr);
                candidateNode->reduceCpuCapacity(1);
            }
        }
    }
}

}

