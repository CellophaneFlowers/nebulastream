#ifndef INCLUDE_ACTORS_WORKERSERVICE_HPP_
#define INCLUDE_ACTORS_WORKERSERVICE_HPP_

#include <NodeEngine/NodeEngine.hpp>
#include <Operators/Operator.hpp>
#include <string>
#include <vector>
#include <tuple>
#include <Catalogs/PhysicalStreamConfig.hpp>

using std::string;
using std::vector;
using std::tuple;
using JSON = nlohmann::json;

namespace NES {

class WorkerService {

  public:

    ~WorkerService()=default;

    /**
     * @brief constructor for worker with a stream/sensor attached and create default physical stream
     */
    WorkerService(string ip, uint16_t publish_port, uint16_t receive_port);


    /**
     * @brief framework internal method which is called to execute a query or sub-query on a node
     * @param queryId a queryId of the query
     * @param executableTransferObject wrapper object with the schema, sources, destinations, operator
     */
    void execute_query(const string& queryId, string& executableTransferObject);

    /**
     * @brief method which is called to unregister an already running query
     * @param query the description of the query
     */
    void delete_query(const string& query);

    /**
     * @brief gets the currently locally running operators and returns them as flattened strings in a vector
     * @return the flattend vector<string> object of operators
     */
    vector<string> getOperators();

    const string& getIp() const;
    void setIp(const string& ip);
    uint16_t getPublishPort() const;
    void setPublishPort(uint16_t publish_port);
    uint16_t getReceivePort() const;
    void setReceivePort(uint16_t receive_port);

    string getNodeProperties();
    PhysicalStreamConfig getPhysicalStreamConfig(std::string name);
    void addPhysicalStreamConfig(PhysicalStreamConfig conf);

  private:
    string _ip;
    uint16_t _publish_port;
    uint16_t _receive_port;
    //TODO: this should be a ref or a pointer instead of an object
    std::map<std::string, PhysicalStreamConfig> physicalStreams;

    NodeEngine* _enginePtr;
    std::unordered_map<string, tuple<QueryExecutionPlanPtr, OperatorPtr>> _runningQueries;
};

}

#endif //INCLUDE_ACTORS_WORKERSERVICE_HPP_
