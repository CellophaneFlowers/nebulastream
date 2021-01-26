/*
    Copyright (C) 2020 by the NebulaStream project (https://nebula.stream)

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

#include <API/Query.hpp>
#include <Catalogs/StreamCatalog.hpp>
#include <Nodes/Expressions/ArithmeticalExpressions/AddExpressionNode.hpp>
#include <Nodes/Expressions/ConstantValueExpressionNode.hpp>
#include <Nodes/Expressions/FieldAccessExpressionNode.hpp>
#include <Nodes/Expressions/FieldAssignmentExpressionNode.hpp>
#include <Operators/LogicalOperators/FilterLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/LogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/MapLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sinks/FileSinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sinks/SinkLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sources/DefaultSourceDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/SourceLogicalOperatorNode.hpp>
#include <Phases/TypeInferencePhase.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <Topology/TopologyNode.hpp>
#include <Util/Logger.hpp>
#include <Util/UtilityFunctions.hpp>
#include <Windowing/TimeCharacteristic.hpp>
#include <Windowing/WindowAggregations/SumAggregationDescriptor.hpp>
#include <Windowing/WindowAggregations/WindowAggregationDescriptor.hpp>
#include <Windowing/WindowTypes/TumblingWindow.hpp>
#include <Windowing/WindowTypes/WindowType.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>

using namespace NES::API;
using namespace NES::Windowing;

namespace NES {

class TypeInferencePhaseTest : public testing::Test {
  public:
    /* Will be called before any test in this class are executed. */
    static void SetUpTestCase() {
        NES::setupLogging("TypeInferencePhaseTest.log", NES::LOG_DEBUG);
        NES_INFO("Setup TypeInferencePhaseTest test class.");
    }

    /* Will be called before a  test is executed. */
    void SetUp() { NES_INFO("Setup TypeInferencePhaseTest test case."); }

    /* Will be called before a test is executed. */
    void TearDown() { NES_INFO("Tear down TypeInferencePhaseTest test case."); }

    /* Will be called after all tests in this class are finished. */
    static void TearDownTestCase() { NES_INFO("Tear down TypeInferencePhaseTest test class."); }
};

/**
 * @brief In this test we infer the output and input schemas of each operator in a query.
 */
TEST_F(TypeInferencePhaseTest, inferQueryPlan) {
    auto inputSchema = Schema::create();
    inputSchema->addField("f1", BasicType::INT32);
    inputSchema->addField("f2", BasicType::INT8);

    auto source =
        LogicalOperatorFactory::createSourceOperator(DefaultSourceDescriptor::create(inputSchema, "default_logical", 0, 0));
    auto map = LogicalOperatorFactory::createMapOperator(Attribute("f3") = Attribute("f1") * 42);
    auto sink = LogicalOperatorFactory::createSinkOperator(FileSinkDescriptor::create(""));

    auto plan = QueryPlan::create(source);
    plan->appendOperatorAsNewRoot(map);
    plan->appendOperatorAsNewRoot(sink);

    StreamCatalogPtr streamCatalog = std::make_shared<StreamCatalog>();
    TopologyNodePtr physicalNode = TopologyNode::create(1, "localhost", 4000, 4002, 4);

    PhysicalStreamConfigPtr streamConf = PhysicalStreamConfig::create();
    StreamCatalogEntryPtr sce = std::make_shared<StreamCatalogEntry>(streamConf, physicalNode);
    streamCatalog->addPhysicalStream("default_logical", sce);

    auto phase = TypeInferencePhase::create(streamCatalog);
    auto resultPlan = phase->execute(plan);

    // we just access the old references

    ASSERT_TRUE(source->getOutputSchema()->equals(inputSchema));

    auto mappedSchema = Schema::create();
    mappedSchema->addField("f1", BasicType::INT32);
    mappedSchema->addField("f2", BasicType::INT8);
    mappedSchema->addField("f3", BasicType::INT8);

    ASSERT_TRUE(map->getOutputSchema()->equals(mappedSchema));
    ASSERT_TRUE(sink->getOutputSchema()->equals(mappedSchema));
}

/**
 * @brief In this test we infer the output and input schemas of each operator in a query.
 */
TEST_F(TypeInferencePhaseTest, inferWindowQuery) {

    auto query = Query::from("default_logical")
                     .windowByKey(Attribute("id"), TumblingWindow::of(TimeCharacteristic::createIngestionTime(), Seconds(10)),
                                  Sum(Attribute("value")))
                     .sink(FileSinkDescriptor::create(""));

    StreamCatalogPtr streamCatalog = std::make_shared<StreamCatalog>();
    TopologyNodePtr physicalNode = TopologyNode::create(1, "localhost", 4000, 4002, 4);

    PhysicalStreamConfigPtr streamConf = PhysicalStreamConfig::create();
    StreamCatalogEntryPtr sce = std::make_shared<StreamCatalogEntry>(streamConf, physicalNode);
    streamCatalog->addPhysicalStream("default_logical", sce);

    auto phase = TypeInferencePhase::create(streamCatalog);
    auto resultPlan = phase->execute(query.getQueryPlan());

    std::cout << resultPlan->getSinkOperators()[0]->getOutputSchema()->toString() << std::endl;
    // we just access the old references
    ASSERT_EQ(resultPlan->getSinkOperators()[0]->getOutputSchema()->getSize(), 5);
}

/**
 * @brief In this test we try to infer the output and input scheas of an invalid query. This should fail.
 */
TEST_F(TypeInferencePhaseTest, inferQueryPlanError) {

    auto inputSchema = Schema::create();
    inputSchema->addField("f1", BasicType::INT32);
    inputSchema->addField("f2", BasicType::INT8);

    auto source =
        LogicalOperatorFactory::createSourceOperator(DefaultSourceDescriptor::create(inputSchema, "default_logical", 0, 0));
    auto map = LogicalOperatorFactory::createMapOperator(Attribute("f3") = Attribute("f3") * 42);
    auto sink = LogicalOperatorFactory::createSinkOperator(FileSinkDescriptor::create(""));

    auto plan = QueryPlan::create(source);
    plan->appendOperatorAsNewRoot(map);
    plan->appendOperatorAsNewRoot(sink);

    StreamCatalogPtr streamCatalog = std::make_shared<StreamCatalog>();
    TopologyNodePtr physicalNode = TopologyNode::create(1, "localhost", 4000, 4002, 4);

    PhysicalStreamConfigPtr streamConf = PhysicalStreamConfig::create();

    StreamCatalogEntryPtr sce = std::make_shared<StreamCatalogEntry>(streamConf, physicalNode);
    streamCatalog->addPhysicalStream("default_logical", sce);
    auto phase = TypeInferencePhase::create(streamCatalog);
    ASSERT_ANY_THROW(phase->execute(plan));
}

/**
 * @brief In this test we ensure that the source descriptor is correctly replaced, such that the schema can be propagated.
 */
TEST_F(TypeInferencePhaseTest, inferQuerySourceReplace) {

    TopologyNodePtr physicalNode = TopologyNode::create(1, "localhost", 4000, 4002, 4);

    PhysicalStreamConfigPtr streamConf = PhysicalStreamConfig::create();
    StreamCatalogPtr streamCatalog = std::make_shared<StreamCatalog>();
    StreamCatalogEntryPtr sce = std::make_shared<StreamCatalogEntry>(streamConf, physicalNode);
    streamCatalog->addPhysicalStream("default_logical", sce);

    SchemaPtr schema = Schema::create()->addField("id", BasicType::UINT32)->addField("value", BasicType::UINT64);

    auto query = Query::from("default_logical").map(Attribute("f3") = Attribute("id")++).sink(FileSinkDescriptor::create(""));
    auto plan = query.getQueryPlan();

    auto phase = TypeInferencePhase::create(streamCatalog);

    plan = phase->execute(plan);
    auto sink = plan->getSinkOperators()[0];

    auto resultSchema = Schema::create()
                            ->addField("id", BasicType::UINT32)
                            ->addField("value", BasicType::UINT64)
                            ->addField("f3", BasicType::UINT32);

    NES_INFO(sink->getOutputSchema()->toString());

    ASSERT_TRUE(sink->getOutputSchema()->equals(resultSchema));
}

/**
 * @brief In this test we ensure that the schema can be propagated properly when merge operator is present.
 */
TEST_F(TypeInferencePhaseTest, inferQueryWithMergeOperator) {

    TopologyNodePtr physicalNode = TopologyNode::create(1, "localhost", 4000, 4002, 4);

    PhysicalStreamConfigPtr streamConf = PhysicalStreamConfig::create();
    StreamCatalogPtr streamCatalog = std::make_shared<StreamCatalog>();
    StreamCatalogEntryPtr sce = std::make_shared<StreamCatalogEntry>(streamConf, physicalNode);
    streamCatalog->addPhysicalStream("default_logical", sce);

    SchemaPtr schema = Schema::create()->addField("id", BasicType::UINT32)->addField("value", BasicType::UINT64);

    Query subQuery = Query::from("default_logical");
    auto query = Query::from("default_logical").merge(&subQuery).map(Attribute("f3") = Attribute("id")++).sink(FileSinkDescriptor::create(""));
    auto plan = query.getQueryPlan();

    auto phase = TypeInferencePhase::create(streamCatalog);

    plan = phase->execute(plan);
    auto sink = plan->getSinkOperators()[0];

    auto resultSchema = Schema::create()
        ->addField("id", BasicType::UINT32)
        ->addField("value", BasicType::UINT64)
        ->addField("f3", BasicType::UINT32);

    NES_INFO(sink->getOutputSchema()->toString());

    ASSERT_TRUE(sink->getOutputSchema()->equals(resultSchema));
}

/**
 * @brief In this test we test the rename operator
 */
TEST_F(TypeInferencePhaseTest, inferQueryRenameBothAttributes) {

    auto inputSchema = Schema::create();
    inputSchema->addField("f1", BasicType::INT32);
    inputSchema->addField("f2", BasicType::INT8);

    auto source =
        LogicalOperatorFactory::createSourceOperator(DefaultSourceDescriptor::create(inputSchema, "default_logical", 0, 0));
    auto map = LogicalOperatorFactory::createMapOperator(Attribute("f3").rename("f4") = Attribute("f3").rename("f5") * 42);
    auto sink = LogicalOperatorFactory::createSinkOperator(FileSinkDescriptor::create(""));

    auto plan = QueryPlan::create(source);
    plan->appendOperatorAsNewRoot(map);
    plan->appendOperatorAsNewRoot(sink);

    StreamCatalogPtr streamCatalog = std::make_shared<StreamCatalog>();
    TopologyNodePtr physicalNode = TopologyNode::create(1, "localhost", 4000, 4002, 4);

    PhysicalStreamConfigPtr streamConf = PhysicalStreamConfig::create();

    StreamCatalogEntryPtr sce = std::make_shared<StreamCatalogEntry>(streamConf, physicalNode);
    streamCatalog->addPhysicalStream("default_logical", sce);
    auto phase = TypeInferencePhase::create(streamCatalog);
    ASSERT_ANY_THROW(phase->execute(plan));
}

/**
 * @brief In this test we test the rename operator
 */
TEST_F(TypeInferencePhaseTest, inferQueryRenameOneAttribute) {

    auto inputSchema = Schema::create();
    inputSchema->addField("f1", BasicType::INT32);
    inputSchema->addField("f2", BasicType::INT8);

    auto source =
        LogicalOperatorFactory::createSourceOperator(DefaultSourceDescriptor::create(inputSchema, "default_logical", 0, 0));
    auto map = LogicalOperatorFactory::createMapOperator(Attribute("f3").rename("f4") = Attribute("f3") * 42);
    auto sink = LogicalOperatorFactory::createSinkOperator(FileSinkDescriptor::create(""));

    auto plan = QueryPlan::create(source);
    plan->appendOperatorAsNewRoot(map);
    plan->appendOperatorAsNewRoot(sink);

    StreamCatalogPtr streamCatalog = std::make_shared<StreamCatalog>();
    TopologyNodePtr physicalNode = TopologyNode::create(1, "localhost", 4000, 4002, 4);

    PhysicalStreamConfigPtr streamConf = PhysicalStreamConfig::create();

    StreamCatalogEntryPtr sce = std::make_shared<StreamCatalogEntry>(streamConf, physicalNode);
    streamCatalog->addPhysicalStream("default_logical", sce);
    auto phase = TypeInferencePhase::create(streamCatalog);
    ASSERT_ANY_THROW(phase->execute(plan));
}

/**
     * @brief In this test we test the rename operator
     */
TEST_F(TypeInferencePhaseTest, inferQueryRenameinAssignment) {

    auto inputSchema = Schema::create();
    inputSchema->addField("f1", BasicType::INT32);
    inputSchema->addField("f2", BasicType::INT8);

    auto source =
        LogicalOperatorFactory::createSourceOperator(DefaultSourceDescriptor::create(inputSchema, "default_logical", 0, 0));
    auto map = LogicalOperatorFactory::createMapOperator(Attribute("f3").rename("f4") = 42);
    auto sink = LogicalOperatorFactory::createSinkOperator(FileSinkDescriptor::create(""));

    auto plan = QueryPlan::create(source);
    plan->appendOperatorAsNewRoot(map);
    plan->appendOperatorAsNewRoot(sink);

    StreamCatalogPtr streamCatalog = std::make_shared<StreamCatalog>();
    TopologyNodePtr physicalNode = TopologyNode::create(1, "localhost", 4000, 4002, 4);

    PhysicalStreamConfigPtr streamConf = PhysicalStreamConfig::create();

    StreamCatalogEntryPtr sce = std::make_shared<StreamCatalogEntry>(streamConf, physicalNode);
    streamCatalog->addPhysicalStream("default_logical", sce);
    auto phase = TypeInferencePhase::create(streamCatalog);
    auto maps = plan->getOperatorByType<MapLogicalOperatorNode>();
    phase->execute(plan);
    NES_DEBUG("result schema is=" << maps[0]->getOutputSchema()->toString());
    //we have to forbit the renaming of the attribute in the assignment statement of the map
    ASSERT_NE(maps[0]->getOutputSchema()->getIndex("f4"), 2);
}

/**
     * @brief In this test we test the rename operator
     */
TEST_F(TypeInferencePhaseTest, inferQueryRenameinAccess) {

    auto inputSchema = Schema::create();
    inputSchema->addField("f1", BasicType::INT32);
    inputSchema->addField("f2", BasicType::INT8);

    auto source =
        LogicalOperatorFactory::createSourceOperator(DefaultSourceDescriptor::create(inputSchema, "default_logical", 0, 0));
    auto filter = LogicalOperatorFactory::createFilterOperator(Attribute("f2").rename("f4") < 42);
    auto sink = LogicalOperatorFactory::createSinkOperator(FileSinkDescriptor::create(""));

    auto plan = QueryPlan::create(source);
    plan->appendOperatorAsNewRoot(filter);
    plan->appendOperatorAsNewRoot(sink);

    StreamCatalogPtr streamCatalog = std::make_shared<StreamCatalog>();
    TopologyNodePtr physicalNode = TopologyNode::create(1, "localhost", 4000, 4002, 4);

    PhysicalStreamConfigPtr streamConf = PhysicalStreamConfig::create();

    StreamCatalogEntryPtr sce = std::make_shared<StreamCatalogEntry>(streamConf, physicalNode);
    streamCatalog->addPhysicalStream("default_logical", sce);
    auto phase = TypeInferencePhase::create(streamCatalog);
    auto filterNode = plan->getOperatorByType<FilterLogicalOperatorNode>();
    phase->execute(plan);
    NES_DEBUG("schema=" << filterNode[0]->getOutputSchema()->toString());
    ASSERT_TRUE(filterNode[0]->getOutputSchema()->getIndex("f4") == 1);
}
}// namespace NES
