#ifndef IOTDB_INCLUDE_ACTORS_COORDINATORACTOR_HPP_
#define IOTDB_INCLUDE_ACTORS_COORDINATORACTOR_HPP_

#include <caf/io/all.hpp>
#include <caf/all.hpp>
#include <Actors/AtomUtils.hpp>
#include <Services/CoordinatorService.hpp>
#include <Services/WorkerService.hpp>
#include <NodeEngine/NodeEngine.hpp>
#include <Actors/Configurations/CoordinatorActorConfig.hpp>
#include <Topology/NESTopologyEntry.hpp>
#include <Topology/NESTopologyManager.hpp>
#include <Catalogs/PhysicalStreamConfig.hpp>
#include <API/Schema.hpp>
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::unordered_map;

namespace iotdb {

// class-based, statically typed, event-based API for the state management in CAF
struct CoordinatorState {
  unordered_map<caf::strong_actor_ptr, NESTopologyEntryPtr> actorTopologyMap;
  unordered_map<NESTopologyEntryPtr, caf::strong_actor_ptr> topologyActorMap;
};

/**
 * @brief the coordinator for NES
 */
class CoordinatorActor : public caf::stateful_actor<CoordinatorState> {

 public:
  /**
   * @brief the constructor of the coordinator to initialize the default objects
   */
  explicit CoordinatorActor(caf::actor_config &cfg)
      :
      stateful_actor(cfg) {
    coordinatorServicePtr = CoordinatorService::getInstance();
    workerServicePtr = std::make_unique<WorkerService>(
        WorkerService(actorCoordinatorConfig.ip,
                      actorCoordinatorConfig.publish_port,
                      actorCoordinatorConfig.receive_port));
  }

  behavior_type make_behavior() override {
    return init();
  }

 private:
  caf::behavior init();
  caf::behavior running();

  /**
   * @brief method to add a logical stream
   * @param logical stream name
   * @param schema of logical stream
   * @return bool indicating if insert was successful
   */
  bool registerLogicalStream(std::string logicalStreamName,
                             SchemaPtr schemaPtr);

  /**
   * @brief method to remove a logical stream
   * @caution this does not remove the corresponding physical streams
   * @caution will fail if there are pyhsical streams for this logical entry, delete them first
   * @param logical stream name
   * @return bool indicating if removal was successful
   */
  bool removeLogicalStream(std::string logicalStreamName);

  /**
   * @brief method to add a physical stream to the catalog AND to the topology
   * @caution every external register call can only register streams for himself
   * @caution we will deploy on the first worker with that ip NOT on all
   * * TODO: maybe add the actor id to make it more specific
   * @param ip as string
   * @param config of the physical stream
   * @return bool indicating if removal was successful
   */
  bool registerPhysicalStream(std::string ip, PhysicalStreamConfig streamConf);

  /**
   * @brief method to remove a physical stream to the catalog AND to the topology
   * @caution every external register call can only remove streams from himself
   * @caution we will remove only on the first node that we find for this
   * TODO: maybe add the actor id to make it more specific
   * @param ip as string
   * @param config of the physical stream
   * @return bool indicating if removal was successful
   */
  bool removePhysicalStream(std::string ip, PhysicalStreamConfig streamConf);

  /**
   * @brief : registering a new sensor node
   * @param ip
   * @param publish_port
   * @param receive_port
   * @param cpu
   * @param properties of this worker
   * @param configuration of the sensor
   */
  void registerSensor(const string &ip, uint16_t publish_port,
                      uint16_t receive_port, int cpu,
                      const string &nodeProperties,
                      PhysicalStreamConfig streamConf);

  /**
   * @brief: remove a sensor node from topology and catalog
   * @caution: if there is more than one, potential node to delete, an exception is thrown
   * @param ip
   */
  bool deregisterSensor(const string &ip);

  /**
   * @brief execute user query will first register the query and then deploy it.
   * @param queryString : user query, in string form, to be executed
   * @param strategy : deployment strategy for the query operators
   * @return UUID of the submitted user query.
   */
  string executeQuery(const string &queryString, const string &strategy);

  /**
   * @brief register the user query
   * @param queryString string representation of the query
   * @return uuid of the registered query
   */
  string registerQuery(const string &queryString, const string &strategy);

  /**
   * @brief: deploy the user query
   *
   * @param queryId
   */
  void deployQuery(const string &queryId);

  /**
   * @brief method which is called to unregister an already running query
   * @param queryId of the query to be de-registered
   * @bool indicating the success of the disconnect
   */
  void deregisterQuery(const string &queryId);

  /**
   * @brief send messages to all connected devices and get their operators
   */
  void showOperators();

  /**
   * @brief initialize the NES topology and add coordinator node
   */
  void initializeNESTopology();

  CoordinatorActorConfig actorCoordinatorConfig;
  CoordinatorServicePtr coordinatorServicePtr;
  std::unique_ptr<WorkerService> workerServicePtr;
};
}

#endif //IOTDB_INCLUDE_ACTORS_COORDINATORACTOR_HPP_
