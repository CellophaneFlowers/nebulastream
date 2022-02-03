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

#ifndef NES_INCLUDE_MONITORING_METRICS_METRICCATALOG_HPP_
#define NES_INCLUDE_MONITORING_METRICS_METRICCATALOG_HPP_

#include <Monitoring/MetricValues/MetricValueType.hpp>
#include <map>
#include <memory>
#include <string>

namespace NES {
class Metric;
class MetricCatalog;
using MetricCatalogPtr = std::shared_ptr<MetricCatalog>;

class MetricCatalog {
  public:
    static MetricCatalogPtr create(std::map<MetricValueType, Metric> metrics);

    static MetricCatalogPtr NesMetrics();

    /**
     * @brief Registers a metric.
     * @param name of the metric
     * @param metric metric to register
     * @return true if successful, else false
     */
    bool add(MetricValueType type, const Metric&& metric);

  private:
    explicit MetricCatalog(std::map<MetricValueType, Metric> metrics);

    std::map<MetricValueType, Metric> metricValueTypeToMetricMap;
};

}// namespace NES

#endif  // NES_INCLUDE_MONITORING_METRICS_METRICCATALOG_HPP_
