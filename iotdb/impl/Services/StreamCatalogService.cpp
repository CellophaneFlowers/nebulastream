#include <Util/Logger.hpp>
#include "Services/StreamCatalogService.hpp"

namespace NES {

bool StreamCatalogService::addNewLogicalStream(const std::string& streamName, const std::string& streamSchema) {
    SchemaPtr schema = UtilityFunctions::createSchemaFromCode(streamSchema);
    NES_DEBUG("StreamCatalogService: schema successfully created")
    return StreamCatalog::instance().addLogicalStream(streamName, schema);
}

bool StreamCatalogService::removeLogicalStream(const std::string& streamName) {
    return StreamCatalog::instance().removeLogicalStream(streamName);
}

std::map<std::string, SchemaPtr> StreamCatalogService::getAllLogicalStream() {
    return StreamCatalog::instance().getAllLogicalStream();
}
std::map<std::string, std::string> StreamCatalogService::getAllLogicalStreamAsString() {

    std::map<std::string, std::string> allLogicalStreamAsString;
    const map<std::string, SchemaPtr> allLogicalStream = getAllLogicalStream();

    for (auto const&[key, val] : allLogicalStream) {
        allLogicalStreamAsString[key] = val->toString();
    }
    return allLogicalStreamAsString;
}

bool StreamCatalogService::updatedLogicalStream(std::string& streamName, std::string& streamSchema) {
    SchemaPtr schema = UtilityFunctions::createSchemaFromCode(streamSchema);
    removeLogicalStream(streamName);
    return StreamCatalog::instance().addLogicalStream(streamName, schema);
}

vector<StreamCatalogEntryPtr> StreamCatalogService::getAllPhysicalStream(const std::string logicalStreamName) {
    return StreamCatalog::instance().getPhysicalStreams(logicalStreamName);
}

}
