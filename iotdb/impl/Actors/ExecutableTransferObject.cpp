#include <Actors/ExecutableTransferObject.hpp>
#include <Operators/Impl/WindowOperator.hpp>
#include <QueryCompiler/QueryCompiler.hpp>
#include <Util/Logger.hpp>

using std::string;
using std::vector;

BOOST_CLASS_EXPORT(NES::ExecutableTransferObject);

namespace NES {
ExecutableTransferObject::ExecutableTransferObject(string queryId,
                                                   const Schema& schema,
                                                   vector<DataSourcePtr> sources,
                                                   vector<DataSinkPtr> destinations,
                                                   OperatorPtr operatorTree) {
    this->queryId = std::move(queryId);
    this->schema = schema;
    this->sources = std::move(sources);
    this->destinations = std::move(destinations);
    this->operatorTree = std::move(operatorTree);
}

WindowDefinitionPtr assignWindowHandler(OperatorPtr operator_ptr) {
    for (OperatorPtr c: operator_ptr->getChildren()) {
        if (auto* windowOpt = dynamic_cast<WindowOperator*>(operator_ptr.get())) {
            return windowOpt->getWindowDefinition();
        }
    }
    return nullptr;
};

QueryExecutionPlanPtr ExecutableTransferObject::toQueryExecutionPlan(QueryCompilerPtr queryCompiler) {
    if (!compiled) {
        this->compiled = true;
        NES_INFO("*** Creating QueryExecutionPlan for " << this->queryId);
        QueryExecutionPlanPtr qep = queryCompiler->compile(this->operatorTree);

        //TODO: currently only one input source is supported
        if (!this->sources.empty()) {
            qep->addDataSource(this->sources[0]);
        } else {
            NES_ERROR("The query " << this->queryId << " has no input sources!")
        }

        if (!this->destinations.empty()) {
            qep->addDataSink(this->destinations[0]);
        } else {
            NES_ERROR("The query " << this->queryId << " has no destinations!")
        }

        return qep;
    } else {
        NES_ERROR(this->queryId + " has already been compiled and cannot be recreated!")
    }
}

string& ExecutableTransferObject::getQueryId() {
    return this->queryId;
}

void ExecutableTransferObject::setQueryId(const string& queryId) {
    this->queryId = queryId;
}

Schema& ExecutableTransferObject::getSchema() {
    return this->schema;
}

void ExecutableTransferObject::setSchema(const Schema& schema) {
    this->schema = schema;
}

vector<DataSourcePtr>& ExecutableTransferObject::getSources() {
    return this->sources;
}

void ExecutableTransferObject::setSources(const vector<DataSourcePtr>& sources) {
    this->sources = sources;
}

vector<DataSinkPtr>& ExecutableTransferObject::getDestinations() {
    return this->destinations;
}

void ExecutableTransferObject::setDestinations(const vector<DataSinkPtr>& destinations) {
    this->destinations = destinations;
}

OperatorPtr& ExecutableTransferObject::getOperatorTree() {
    return this->operatorTree;
}

void ExecutableTransferObject::setOperatorTree(const OperatorPtr& operatorTree) {
    this->operatorTree = operatorTree;
}
}
