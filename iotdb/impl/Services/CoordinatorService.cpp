#include <Services/CoordinatorService.hpp>
#include <Actors/ExecutableTransferObject.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <SourceSink/SourceCreator.hpp>
#include <Util/Logger.hpp>
#include <Catalogs/QueryCatalog.hpp>

using namespace NES;
using namespace std;

string CoordinatorService::getNodePropertiesAsString(const NESTopologyEntryPtr entry) {
    return entry->getNodeProperty();
}

NESTopologyEntryPtr CoordinatorService::register_sensor(size_t id, const string& ip, uint16_t publish_port,
                                                        uint16_t receive_port, int cpu, const string& nodeProperties,
                                                        PhysicalStreamConfig streamConf) {

    NESTopologyManager& topologyManager = this->topologyManagerPtr->getInstance();
    NESTopologySensorNodePtr sensorNode = topologyManager.createNESSensorNode(id, ip, CPUCapacity::Value(cpu));

    sensorNode->setPhysicalStreamName(streamConf.physicalStreamName);
    sensorNode->setPublishPort(publish_port);
    sensorNode->setReceivePort(receive_port);
    if (nodeProperties != "defaultProperties")
        sensorNode->setNodeProperty(nodeProperties);

    NES_DEBUG(
        "try to register sensor phyName=" << streamConf.physicalStreamName << " logName="
                                          << streamConf.logicalStreamName << " nodeID=" << sensorNode->getId())
    //check if logical stream exists
    if (!StreamCatalog::instance().testIfLogicalStreamExistsInSchemaMapping(
        streamConf.logicalStreamName)) {
        NES_ERROR(
            "Coordinator: error logical stream" << streamConf.logicalStreamName
                                                << " does not exist when adding physical stream "
                                                << streamConf.physicalStreamName)
        throw Exception(
            "logical stream does not exist " + streamConf.logicalStreamName);
    }
    SchemaPtr schema = StreamCatalog::instance().getSchemaForLogicalStream(
        streamConf.logicalStreamName);

    DataSourcePtr source;
    if (streamConf.sourceType != "CSVSource"
        && streamConf.sourceType != "DefaultSource") {
        NES_ERROR(
            "Coordinator: error source type " << streamConf.sourceType << " is not supported")
        throw Exception(
            "Coordinator: error source type " + streamConf.sourceType
                + " is not supported");
    }
    StreamCatalogEntryPtr sce = std::make_shared<StreamCatalogEntry>(
        streamConf.sourceType, streamConf.sourceConfig, sensorNode,
        streamConf.physicalStreamName);

    bool success = StreamCatalog::instance().addPhysicalStream(
        streamConf.logicalStreamName, sce);
    if (!success) {
        NES_ERROR(
            "Coordinator: physical stream " << streamConf.physicalStreamName << " could not be added to catalog")
        throw Exception(
            "Coordinator: physical stream " + streamConf.physicalStreamName
                + " could not be added to catalog");

    }

    const NESTopologyEntryPtr kRootNode = NESTopologyManager::getInstance()
        .getRootNode();

    // FIXME: decide how to send the link capacity and link latency values
    topologyManager.createNESTopologyLink(sensorNode, kRootNode, 1, 1);
    return sensorNode;
}

string CoordinatorService::registerQuery(
    const string& queryString, const string& optimizationStrategyName) {
    return QueryCatalog::instance().registerQuery(queryString,
                                                  optimizationStrategyName);
}

bool CoordinatorService::deleteQuery(const string& queryId) {
    return QueryCatalog::instance().deleteQuery(queryId);
}

map<NESTopologyEntryPtr, ExecutableTransferObject> CoordinatorService::prepareExecutableTransferObject(
    const string& queryId) {
    map<NESTopologyEntryPtr, ExecutableTransferObject> output;
    if (QueryCatalog::instance().queryExists(queryId)
        && !QueryCatalog::instance().isQueryRunning(queryId)) {
        NES_INFO("CoordinatorService: Deploying query " << queryId);

        NESExecutionPlanPtr execPlan = QueryCatalog::instance().getQuery(queryId)
            ->getNesPlanPtr();

        Schema schema = QueryCatalog::instance().getQuery(queryId)->getInputQueryPtr()
            ->getSourceStream()->getSchema();

        //iterate through all vertices in the topology
        for (const ExecutionVertex& v : execPlan->getExecutionGraph()->getAllVertex()) {
            OperatorPtr operators = v.ptr->getRootOperator();
            if (operators) {
                // if node contains operators to be deployed -> serialize and send them to the according node
                vector<DataSourcePtr> sources = getSources(queryId, v);
                vector<DataSinkPtr> destinations = getSinks(queryId, v);
                NESTopologyEntryPtr nesNode = v.ptr->getNESNode();
                //TODO: this will break for multiple streams
                ExecutableTransferObject eto = ExecutableTransferObject(queryId, schema,
                                                                        sources,
                                                                        destinations,
                                                                        operators);
                output.insert({nesNode, eto});
            }
        }

        QueryCatalog::instance().markQueryAs(queryId, QueryStatus::Scheduling);

    } else if (QueryCatalog::instance().getQuery(queryId)->getQueryStatus() == QueryStatus::Running) {
        NES_WARNING("CoordinatorService: Query is already running -> " << queryId);
    } else {
        NES_WARNING("CoordinatorService: Query is not registered -> " << queryId);
    }
    return output;
}

vector<DataSourcePtr> CoordinatorService::getSources(const string& queryId,
                                                     const ExecutionVertex& v) {
    vector<DataSourcePtr> out = vector<DataSourcePtr>();
    NESExecutionPlanPtr execPlan = QueryCatalog::instance().getQuery(queryId)
        ->getNesPlanPtr();
    Schema schema = QueryCatalog::instance().getQuery(queryId)->getInputQueryPtr()
        ->getSourceStream()->getSchema();

    DataSourcePtr source = findDataSourcePointer(v.ptr->getRootOperator());

    //FIXME: what about user defined a ZMQ sink?
    if (source->getType() == ZMQ_SOURCE) {
        //FIXME: Maybe a better way to do it? perhaps type cast to ZMQSource type and just update the port number
        // create local zmq source
        const NESTopologyEntryPtr kRootNode = NESTopologyManager::getInstance()
            .getRootNode();
        source = createZmqSource(schema, kRootNode->getIp(), assign_port(queryId));
    }
    out.emplace_back(source);
    return out;
}

vector<DataSinkPtr> CoordinatorService::getSinks(const string& queryId,
                                                 const ExecutionVertex& v) {
    vector<DataSinkPtr> out = vector<DataSinkPtr>();
    //TODO: I am not sure what is happening here with the idx 0 and 1
    NESExecutionPlanPtr execPlan = QueryCatalog::instance().getQuery(queryId)
        ->getNesPlanPtr();
    DataSinkPtr sink = findDataSinkPointer(v.ptr->getRootOperator());

    //FIXME: what about user defined a ZMQ sink?
    if (sink->getType() == ZMQ_SINK) {
        //FIXME: Maybe a better way to do it? perhaps type cast to ZMQSink type and just update the port number
        //create local zmq sink
        const NESTopologyEntryPtr kRootNode = NESTopologyManager::getInstance()
            .getRootNode();

        sink = createZmqSink(sink->getSchema(), kRootNode->getIp(),
                             assign_port(queryId));
    }
    out.emplace_back(sink);
    return out;
}

DataSinkPtr CoordinatorService::findDataSinkPointer(OperatorPtr operatorPtr) {

    if (!operatorPtr->getParent() && operatorPtr->getOperatorType() == SINK_OP) {
        SinkOperator* sinkOperator = dynamic_cast<SinkOperator*>(operatorPtr.get());
        return sinkOperator->getDataSinkPtr();
    } else if (operatorPtr->getParent() != nullptr) {
        return findDataSinkPointer(operatorPtr->getParent());
    } else {
        NES_WARNING("Found query graph without a SINK.");
        return nullptr;
    }
}

DataSourcePtr CoordinatorService::findDataSourcePointer(
    OperatorPtr operatorPtr) {
    vector<OperatorPtr> children = operatorPtr->getChildren();
    if (children.empty() && operatorPtr->getOperatorType() == SOURCE_OP) {
        SourceOperator* sourceOperator = dynamic_cast<SourceOperator*>(operatorPtr
            .get());
        return sourceOperator->getDataSourcePtr();
    } else if (!children.empty()) {
        //FIXME: What if there are more than one source?
        for (OperatorPtr child : children) {
            return findDataSourcePointer(operatorPtr->getParent());
        }
    }
    NES_WARNING("Found query graph without a SOURCE.");
    return nullptr;
}

int CoordinatorService::assign_port(const string& queryId) {
    if (this->queryToPort.find(queryId) != this->queryToPort.end()) {
        return this->queryToPort.at(queryId);
    } else {
        // increase max port in map by 1
        const NESTopologyEntryPtr kRootNode = NESTopologyManager::getInstance()
            .getRootNode();
        uint16_t kFreeZmqPort = kRootNode->getNextFreeReceivePort();
        this->queryToPort.insert({queryId, kFreeZmqPort});
        return kFreeZmqPort;
    }
}

bool CoordinatorService::deregister_sensor(const NESTopologyEntryPtr entry) {

    return this->topologyManagerPtr->getInstance().removeNESNode(entry);

}
string CoordinatorService::getTopologyPlanString() {
    return this->topologyManagerPtr->getInstance().getNESTopologyPlanString();
}

NESExecutionPlanPtr CoordinatorService::getRegisteredQuery(string queryId) {
    if (QueryCatalog::instance().queryExists(queryId)) {
        NES_DEBUG("CoordinatorService: return existing query " << queryId)
        return QueryCatalog::instance().getQuery(queryId)->getNesPlanPtr();
    }
    NES_DEBUG("CoordinatorService: query with id does not exits" << queryId)
    return nullptr;
}

bool CoordinatorService::clearQueryCatalogs() {
    try {
        QueryCatalog::instance().clearQueries();
    } catch (...) {
        assert(0);
        return false;
    }
    return true;
}

const map<string, QueryCatalogEntryPtr> CoordinatorService::getRegisteredQueries() {
    return QueryCatalog::instance().getRegisteredQueries();
}

const map<string, QueryCatalogEntryPtr> CoordinatorService::getRunningQueries() {
    return QueryCatalog::instance().getQueries(QueryStatus::Running);
}
