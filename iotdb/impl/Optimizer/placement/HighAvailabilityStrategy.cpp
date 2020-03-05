#include <Optimizer/placement/HighAvailabilityStrategy.hpp>
#include <Operators/Operator.hpp>
#include <Util/Logger.hpp>
#include <Optimizer/utils/PathFinder.hpp>

namespace NES {

NESExecutionPlanPtr HighAvailabilityStrategy::initializeExecutionPlan(InputQueryPtr inputQuery,
                                                                      NESTopologyPlanPtr nesTopologyPlan) {
    const OperatorPtr sinkOperator = inputQuery->getRoot();

    // FIXME: current implementation assumes that we have only one source stream and therefore only one source operator.
    const string& streamName = inputQuery->getSourceStream()->getName();
    const OperatorPtr sourceOperatorPtr = getSourceOperator(sinkOperator);

    if (!sourceOperatorPtr) {
        NES_ERROR("HighAvailability: Unable to find the source operator.");
        throw std::runtime_error("No source operator found in the query plan");
    }

    const vector<NESTopologyEntryPtr> sourceNodePtrs = StreamCatalog::instance()
        .getSourceNodesForLogicalStream(streamName);

    if (sourceNodePtrs.empty()) {
        NES_ERROR("HighAvailability: Unable to find the target source: " << streamName);
        throw std::runtime_error("No source found in the topology for stream " + streamName);
    }

    NESExecutionPlanPtr nesExecutionPlanPtr = std::make_shared<NESExecutionPlan>();
    const NESTopologyGraphPtr nesTopologyGraphPtr = nesTopologyPlan->getNESTopologyGraph();

    NES_INFO("HighAvailability: Placing operators on the nes topology.");
    placeOperators(nesExecutionPlanPtr, nesTopologyGraphPtr, sourceOperatorPtr, sourceNodePtrs);

    NES_INFO("HighAvailability: Generating complete execution Graph.");
    completeExecutionGraphWithNESTopology(nesExecutionPlanPtr, nesTopologyPlan);

    //FIXME: We are assuming that throughout the pipeline the schema would not change.
    Schema& schema = inputQuery->getSourceStream()->getSchema();
    addSystemGeneratedSourceSinkOperators(schema, nesExecutionPlanPtr);

    return nesExecutionPlanPtr;
}

void HighAvailabilityStrategy::placeOperators(NESExecutionPlanPtr nesExecutionPlanPtr,
                                              NESTopologyGraphPtr nesTopologyGraphPtr,
                                              OperatorPtr sourceOperator,
                                              vector<NESTopologyEntryPtr> sourceNodes) {

    NESTopologyEntryPtr sinkNode = nesTopologyGraphPtr->getRoot();
    PathFinder pathFinder;
    size_t linkRedundency = 2;

    NES_INFO("HighAvailability: Find paths between source nodes and sink node such that the nodes on the paths are"
             "connected with " << linkRedundency << " number of redundant links.");
    vector<vector<NESTopologyEntryPtr>> placementPaths;

    for (NESTopologyEntryPtr sourceNode: sourceNodes) {

        NES_DEBUG("HighAvailability: For each source find all paths between source and sink nodes.");
        const auto listOfPaths = pathFinder.findAllPathsBetween(sourceNode, sinkNode);

        //Find the most common path among the list of paths
        vector<NESTopologyEntryPtr> pathForPlacement;
        size_t maxPathWeight = 0;
        NES_DEBUG("HighAvailability: Find a path such that the path nodes are shared with most of the remaining paths.");
        for (size_t i = 0; i < listOfPaths.size(); i++) {

            vector<NESTopologyEntryPtr> path_i = listOfPaths[i];
            map<NESTopologyEntryPtr, size_t> nodeCountMap;
            for (size_t j = 0; j < listOfPaths.size(); j++) {

                NES_DEBUG("HighAvailability: Skip comparision with itself.");
                if (i == j) {
                    continue;
                }
                vector<NESTopologyEntryPtr> path_j = listOfPaths[j];
                NES_DEBUG("HighAvailability: Fast Forward the path to find first non common node.");
                size_t pathINodeIndex = 0;
                while (path_i[pathINodeIndex]->getId() == path_j[pathINodeIndex]->getId()
                    && pathINodeIndex <= path_i.size()
                    && pathINodeIndex <= path_j.size()) {

                    if (nodeCountMap.find(path_i[pathINodeIndex]) == nodeCountMap.end()) {
                        nodeCountMap[path_i[pathINodeIndex]] = 0;
                    }
                    pathINodeIndex++;
                }

                NES_DEBUG(
                    "HighAvailability: If no non-common node exists with current path then skip to compare with next path.");
                if (pathINodeIndex > path_i.size() || pathINodeIndex > path_j.size()) {
                    continue;
                }

                for (size_t idx = pathINodeIndex; idx < path_i.size(); idx++) {
                    auto node_i = path_i[idx];
                    const auto itr = find_if(path_j.begin(), path_j.end(), [node_i](NESTopologyEntryPtr node_j) {
                      return node_i->getId() == node_j->getId();
                    });

                    if (itr != path_j.end()) {
                        if (nodeCountMap.find(node_i) != nodeCountMap.end()) {
                            nodeCountMap[node_i] = nodeCountMap[node_i] + 1;
                        } else {
                            nodeCountMap[node_i] = 1;
                        }
                    } else if (nodeCountMap.find(node_i) == nodeCountMap.end()) {
                        nodeCountMap[node_i] = 0;
                    }
                }
            }

            size_t totalWeight = 0;
            vector<NESTopologyEntryPtr> commonPath = {sourceNode};
            for (auto itr = nodeCountMap.rbegin(); itr != nodeCountMap.rend(); itr++) {
                NES_DEBUG("HighAvailability: Check if the node on the path is shared by atleast " << linkRedundency
                                                                                                  << " paths.");
                if (itr->second >= linkRedundency - 1) {
                    commonPath.push_back(itr->first);
                    totalWeight++;
                }
            }

            if (maxPathWeight < totalWeight) {
                pathForPlacement = commonPath;
            }
        }
        NES_DEBUG("HighAvailability: Place the path with nodes that are shared with most of the remaining paths.");
        placementPaths.push_back(pathForPlacement);
    }

    NES_INFO("HighAvailability: Sort the paths based on available compute resources.");

    //Sort all the paths with increased aggregated compute capacity
    vector<std::pair<size_t, int>> computeCostList;

    //Calculate total compute cost for each path
    for (size_t i = 0; i < placementPaths.size(); i++) {
        vector<NESTopologyEntryPtr> path = placementPaths[i];
        size_t totalComputeForPath = 0;
        for (NESTopologyEntryPtr node: path) {
            totalComputeForPath = totalComputeForPath + node->getCpuCapacity();
        }
        computeCostList.push_back(make_pair(totalComputeForPath, i));
    }

    sort(computeCostList.begin(), computeCostList.end());

    vector<vector<NESTopologyEntryPtr>> sortedListOfPaths;
    for (auto pair :  computeCostList) {
        sortedListOfPaths.push_back(placementPaths[pair.second]);
    }

    NES_INFO("HighAvailability: Perform placement of operators on each path.");

    for (vector<NESTopologyEntryPtr> pathForPlacement: sortedListOfPaths) {

        OperatorPtr targetOperator = sourceOperator;
        //Perform Bottom-Up placement
        for (size_t i = 0; i < pathForPlacement.size(); i++) {
            NESTopologyEntryPtr node = pathForPlacement[i];
            while (node->getRemainingCpuCapacity() > 0 && targetOperator) {

                if (targetOperator->getOperatorType() == SINK_OP) {
                    node = sinkNode;
                }

                if (!nesExecutionPlanPtr->hasVertex(node->getId())) {
                    NES_DEBUG("HighThroughput: Create new execution node.")
                    stringstream operatorName;
                    operatorName << operatorTypeToString[targetOperator->getOperatorType()] << "(OP-"
                                 << std::to_string(targetOperator->getOperatorId()) << ")";
                    const ExecutionNodePtr newExecutionNode =
                        nesExecutionPlanPtr->createExecutionNode(operatorName.str(), to_string(node->getId()), node,
                                                                 targetOperator->copy());
                    newExecutionNode->addOperatorId(targetOperator->getOperatorId());
                } else {

                    const ExecutionNodePtr existingExecutionNode = nesExecutionPlanPtr
                        ->getExecutionNode(node->getId());
                    size_t operatorId = targetOperator->getOperatorId();
                    vector<size_t>& residentOperatorIds = existingExecutionNode->getChildOperatorIds();
                    const auto exists = std::find(residentOperatorIds.begin(), residentOperatorIds.end(), operatorId);
                    if (exists != residentOperatorIds.end()) {
                        //skip adding rest of the operator chains as they already exists.
                        NES_DEBUG("HighThroughput: skip adding rest of the operator chains as they already exists.");
                        targetOperator = nullptr;
                        break;
                    } else {

                        NES_DEBUG("HighThroughput: adding target operator to already existing operator chain.");
                        stringstream operatorName;
                        operatorName << existingExecutionNode->getOperatorName() << "=>"
                                     << operatorTypeToString[targetOperator->getOperatorType()]
                                     << "(OP-" << std::to_string(targetOperator->getOperatorId()) << ")";
                        existingExecutionNode->addOperator(targetOperator->copy());
                        existingExecutionNode->setOperatorName(operatorName.str());
                        existingExecutionNode->addOperatorId(targetOperator->getOperatorId());
                    }
                }

                targetOperator = targetOperator->getParent();
                node->reduceCpuCapacity(1);
            }

            if (!targetOperator) {
                break;
            }

            NES_INFO("HighAvailability: Find if next target operator already placed on one of the nodes of current path");

            //find if the next target operator already placed
            bool isAlreadyPlaced = false;
            for (size_t j = i + 1; j < pathForPlacement.size(); j++) {
                if (nesExecutionPlanPtr->hasVertex(pathForPlacement[j]->getId())) {
                    vector<size_t> placedOperators =
                        nesExecutionPlanPtr->getExecutionNode(pathForPlacement[j]->getId())->getChildOperatorIds();
                    size_t operatorId = targetOperator->getOperatorId();
                    auto found = find_if(placedOperators.begin(), placedOperators.end(), [operatorId](size_t opId) {
                      return operatorId == opId;
                    });

                    if (found != placedOperators.end()) {
                        isAlreadyPlaced = true;
                    }
                }
            }

            if (isAlreadyPlaced) {
                break;
            }
        }

        NES_INFO("HighAvailability: Add forward operators to the remaining nodes on current path");
        addForwardOperators(pathForPlacement, nesExecutionPlanPtr);
    }
}

void HighAvailabilityStrategy::addForwardOperators(vector<NESTopologyEntryPtr> pathForPlacement,
                                                   NES::NESExecutionPlanPtr nesExecutionPlanPtr) const {

    PathFinder pathFinder;

    for (size_t i = 0; i < pathForPlacement.size() - 1; i++) {

        //Find the list of nodes connecting the source and destination nodes
        vector<vector<NESTopologyEntryPtr>>
            paths = pathFinder.findAllPathsBetween(pathForPlacement[i], pathForPlacement[i + 1]);

        for (vector<NESTopologyEntryPtr> path: paths) {
            for (auto node: path) {
                if (node->getCpuCapacity() == node->getRemainingCpuCapacity()) {
                    nesExecutionPlanPtr->createExecutionNode("FWD",
                                                             to_string(node->getId()),
                                                             node,
                        /**executableOperator**/nullptr);
                    node->reduceCpuCapacity(1);
                }
            }
        }
    }

}

}
