#include "gtest/gtest.h"

#include <iostream>

#include <Catalogs/QueryCatalog.hpp>
#include <API/InputQuery.hpp>
#include <Services/CoordinatorService.hpp>

#include <Topology/NESTopologyManager.hpp>
#include <API/Schema.hpp>

#include <Util/Logger.hpp>
#include <NodeEngine/MemoryLayout/MemoryLayout.hpp>

using namespace NES;

class QueryExecutionTest : public testing::Test {
  public:
    /* Will be called before any test in this class are executed. */
    static void SetUpTestCase() {
        std::cout << "Setup QueryCatalogTest test class." << std::endl;
    }

    /* Will be called before a test is executed. */
    void SetUp() {
        setupLogging();
        std::cout << "Setup QueryCatalogTest test case." << std::endl;
        NES::Dispatcher::instance();
        NES::BufferManager::instance();
        NES::ThreadPool::instance();
        ThreadPool::instance().setNumberOfThreadsWithRestart(1);
        ThreadPool::instance().start();

        // create test input buffer
        testSchema = Schema().create()
            .addField(createField("id", BasicType::INT64))
            .addField(createField("one", BasicType::INT64))
            .addField(createField("value", BasicType::INT64));
        testInputBuffer = BufferManager::instance().getBuffer();
        memoryLayout = createRowLayout(std::make_shared<Schema>(testSchema));
        for (int i = 0; i < 10; i++) {
            memoryLayout->writeField<int64_t>(testInputBuffer, i, 0, i);
            memoryLayout->writeField<int64_t>(testInputBuffer, i, 1, 1);
            memoryLayout->writeField<int64_t>(testInputBuffer, i, 2, i%2);
        }
        testInputBuffer->setNumberOfTuples(10);

    }

    /* Will be called before a test is executed. */
    void TearDown() {
        std::cout << "Tear down QueryCatalogTest test case." << std::endl;
    }

    /* Will be called after all tests in this class are finished. */
    static void TearDownTestCase() {
        std::cout << "Tear down QueryCatalogTest test class." << std::endl;
    }
    TupleBufferPtr testInputBuffer;
    Schema testSchema;
    MemoryLayoutPtr memoryLayout;
  protected:
    static void setupLogging() {
        // create PatternLayout
        log4cxx::LayoutPtr layoutPtr(
            new log4cxx::PatternLayout(
                "%d{MMM dd yyyy HH:mm:ss} %c:%L [%-5t] [%p] : %m%n"));

        // create FileAppender
        LOG4CXX_DECODE_CHAR(fileName, "QueryCatalogTest.log");
        log4cxx::FileAppenderPtr file(
            new log4cxx::FileAppender(layoutPtr, fileName));

        // create ConsoleAppender
        log4cxx::ConsoleAppenderPtr console(
            new log4cxx::ConsoleAppender(layoutPtr));

        // set log level
        NES::NESLogger->setLevel(log4cxx::Level::getDebug());
        //    logger->setLevel(log4cxx::Level::getInfo());

        // add appenders and other will inherit the settings
        NES::NESLogger->addAppender(file);
        NES::NESLogger->addAppender(console);
    }
};

class TestSink : public DataSink {
  public:

    bool writeData(const TupleBufferPtr input_buffer) override {
        NES_DEBUG("TestSink: got buffer " << input_buffer);
        NES_DEBUG(NES::toString(input_buffer.get(), this->getSchema()));
        input_buffer->print();
        resultBuffers.push_back(input_buffer);
        return true;
    }

    const std::string toString() const override {
        return "";
    }

    void setup() override {};
    void shutdown() override {};
    ~TestSink() override {};
    SinkType getType() const override {
        return SinkType::PRINT_SINK;
    }

    uint32_t getNumberOfResultBuffers() {
        return resultBuffers.size();
    }

    std::vector<TupleBufferPtr> resultBuffers;
};

TEST_F(QueryExecutionTest, filterQuery) {

    // creating query plan
    auto testSource = createDefaultDataSourceWithSchemaForOneBuffer(testSchema);
    auto source = createSourceOperator(testSource);
    auto filter = createFilterOperator(createPredicate(Field(testSchema.get("id")) < 5));
    auto testSink = std::make_shared<TestSink>();
    auto sink = createSinkOperator(testSink);

    filter->addChild(source);
    source->setParent(filter);
    sink->addChild(filter);
    filter->setParent(sink);

    auto compiler = createDefaultQueryCompiler();
    auto plan = compiler->compile(sink);
    plan->addDataSink(testSink);
    plan->addDataSource(testSource);

    // The plan should have one pipeline
    EXPECT_EQ(plan->numberOfPipelineStages(), 1);

    plan->executeStage(0, testInputBuffer);

    // This plan should produce one output buffer
    EXPECT_EQ(testSink->getNumberOfResultBuffers(), 1);

    auto resultBuffer = testSink->resultBuffers[0];
    // The output buffer should contain 5 tuple;
    EXPECT_EQ(resultBuffer->getNumberOfTuples(), 5);

    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(memoryLayout->readField<int64_t>(resultBuffer, i, 0), i);
    }
}

TEST_F(QueryExecutionTest, windowQuery) {

    // creating query plan
    auto testSource = createDefaultDataSourceWithSchemaForOneBuffer(testSchema);
    auto source = createSourceOperator(testSource);
    auto aggregation = Sum::on(testSchema.get("one"));
    auto windowType = TumblingWindow::of(TimeCharacteristic::ProcessingTime, Milliseconds(2));
    auto
        windowOperator = createWindowOperator(createWindowDefinition(testSchema.get("value"), aggregation, windowType));
    Schema resultSchema = Schema().create().addField(createField("sum", BasicType::INT64));
    SchemaPtr ptr = std::make_shared<Schema>(resultSchema);
    auto windowScan = createWindowScanOperator(ptr);
    auto testSink = std::make_shared<TestSink>();
    auto sink = createSinkOperator(testSink);

    windowOperator->addChild(source);
    source->setParent(windowOperator);
    windowScan->addChild(windowOperator);
    windowOperator->setParent(windowScan);
    sink->addChild(windowScan);
    windowScan->setParent(sink);

    auto compiler = createDefaultQueryCompiler();
    auto plan = compiler->compile(sink);
    plan->addDataSink(testSink);
    plan->addDataSource(testSource);
    Dispatcher::instance().registerQueryWithoutStart(plan);
    plan->setup();
    plan->start();


    // The plan should have one pipeline
    EXPECT_EQ(plan->numberOfPipelineStages(), 2);
    // TODO switch to event time if that is ready to remove sleep
    // ingest test data
    for (int i = 0; i < 10; i++) {
        plan->executeStage(0, testInputBuffer);
        sleep(1);
    }
    plan->stop();
    sleep(1);

    auto resultBuffer = testSink->resultBuffers[2];
    // The output buffer should contain 5 tuple;
    EXPECT_EQ(resultBuffer->getNumberOfTuples(), 2);
    auto resultLayout = createRowLayout(ptr);
    for (int i = 0; i < 2; i++) {
        EXPECT_EQ(resultLayout->readField<int64_t>(resultBuffer, i, 0), 10);
    }
}



