/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        https://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
#ifndef NES_ABSTRACTSYSTEMRESOURCESREADER_HPP
#define NES_ABSTRACTSYSTEMRESOURCESREADER_HPP

#include <Monitoring/ResourcesReader/SystemResourcesReaderType.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace NES {

class CpuMetricsWrapper;
class MemoryMetrics;
class NetworkMetricsWrapper;
class DiskMetrics;
class RuntimeNesMetrics;
class StaticNesMetrics;

/**
* @brief This is an abstract class so derived classes can collect basic system information
* Warning: Only Linux distributions are currently supported
*/
class AbstractSystemResourcesReader {
  public:
    //  -- Constructors --
    AbstractSystemResourcesReader();
    AbstractSystemResourcesReader(const AbstractSystemResourcesReader&) = default;
    AbstractSystemResourcesReader(AbstractSystemResourcesReader&&) = default;
    //  -- Assignment --
    AbstractSystemResourcesReader& operator=(const AbstractSystemResourcesReader&) = default;
    AbstractSystemResourcesReader& operator=(AbstractSystemResourcesReader&&) = default;
    //  -- dtor --
    virtual ~AbstractSystemResourcesReader() = default;

    /**
    * @brief This methods reads runtime system metrics that are used within NES (e.g., memory usage, cpu load).
    * @return A RuntimeNesMetrics object containing the metrics.
    */
    virtual RuntimeNesMetrics readRuntimeNesMetrics();

    /**
    * @brief This methods reads system metrics that are used within NES (e.g., totalMemoryBytes, core num.).
    * @return A StaticNesMetrics object containing the metrics.
    */
    virtual StaticNesMetrics readStaticNesMetrics();

    /**
    * @brief This method reads CPU information from /proc/stat.
    * Warning: Does not return correct values in containerized environments.
    * @return A map where for each CPU the according /proc/stat information are returned in the form
    * e.g., output["user1"] = 1234, where user is the metric and 1 the cpu core
    */
    virtual CpuMetricsWrapper readCpuStats();

    /**
    * @brief This method reads memory information from sysinfo
    * Warning: Does not return correct values in containerized environments.
    * @return A map with the memory information
    */
    virtual MemoryMetrics readMemoryStats();

    /**
    * @brief This method reads disk stats from statvfs
    * Warning: Does not return correct values in containerized environments.
    * @return A map with the disk stats
    */
    virtual DiskMetrics readDiskStats();

    /**
    * @brief This methods reads network statistics from /proc/net/dev and returns them for each interface in a
    * separate map
    * @return a map where each interface is mapping the according network statistics map.
    */
    virtual NetworkMetricsWrapper readNetworkStats();

    /**
     * @brief Getter for the wall clock time.
    * @return Returns the wall clock time of the system in nanoseconds.
    */
    virtual uint64_t getWallTimeInNs();

    /**
     * @brief Getter for the reader type.
     * @return The SystemResourcesReaderType
     */
    [[nodiscard]] SystemResourcesReaderType getReaderType() const;

  protected:
    SystemResourcesReaderType readerType;
};
using AbstractSystemResourcesReaderPtr = std::shared_ptr<AbstractSystemResourcesReader>;

}// namespace NES

#endif//NES_ABSTRACTSYSTEMRESOURCESREADER_HPP