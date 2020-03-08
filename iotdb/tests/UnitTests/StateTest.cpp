#include <gtest/gtest.h>
#include <Util/Logger.hpp>
#include <log4cxx/appender.h>

#include <State/StateManager.hpp>
#include <State/StateVariable.hpp>

namespace NES {
class StateTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    NES::setupLogging("StateTest.log", NES::LOG_DEBUG);

    NES_INFO("Setup StateTest test class.");
  }
  static void TearDownTestCase() {
    std::cout << "Tear down StateTest test class." << std::endl;
  }
};

TEST_F(StateTest, test_add_clear) {
  StateManager &stateManager = StateManager::instance();
  StateVariable<uint32_t, uint32_t> &var = stateManager
      .registerState<uint32_t, uint32_t>("window-content-0");
  auto kv = var[23];

  EXPECT_EQ(!!kv, false);

  kv.put(43);

  EXPECT_EQ(kv.value(), 43);

  kv.clear();  // unexpected bahavior afterwards

  bool catched = false;
  try {
    EXPECT_NE(kv.value(), 43);
  } catch (std::out_of_range &e) {
    catched = true;
  }
  EXPECT_EQ(catched, true);
}

TEST_F(StateTest, test_emplace_clear) {
  StateManager &stateManager = StateManager::instance();
  StateVariable<uint32_t, uint32_t> &var = stateManager
      .registerState<uint32_t, uint32_t>("window-content-1");
  auto kv = var[23];

  EXPECT_EQ(!!kv, false);

  kv.emplace(43);

  EXPECT_EQ(kv.value(), 43);

  kv.clear();  // unexpected bahavior afterwards

  bool catched = false;
  try {
    EXPECT_NE(kv.value(), 43);
  } catch (std::out_of_range &e) {
    catched = true;
  }
  EXPECT_EQ(catched, true);
}

TEST_F(StateTest, test_multiple_add_lookup) {
  StateManager &stateManager = StateManager::instance();
  StateVariable<uint32_t, uint32_t> &var = stateManager
      .registerState<uint32_t, uint32_t>("window-content-2");

  std::unordered_map<uint32_t, uint32_t> map;

  for (size_t i = 0; i < 8192; i++) {

    uint32_t key = rand();
    uint32_t val = rand();

    var[key] = val;
    map[key] = val;
  }

  for (auto &it : map) {
    auto key = it.first;
    auto val = it.second;
    EXPECT_EQ(var[key].value(), val);
  }
}

TEST_F(StateTest, test_multiple_add_lookup_mt) {
  StateManager &stateManager = StateManager::instance();
  StateVariable<uint32_t, uint32_t> &var = stateManager
      .registerState<uint32_t, uint32_t>("window-content-3");

  std::vector<std::thread> t;

  std::mutex mutex;
  std::unordered_map<uint32_t, uint32_t> map;
  constexpr uint32_t num_threads = 4;
  constexpr uint32_t max_values = 2000000 / num_threads;

  for (uint32_t i = 0; i < num_threads; i++) {
    t.emplace_back([&var, &map, &mutex]() {
      std::unique_lock<std::mutex> lock(mutex, std::defer_lock);
      for (size_t i = 0; i < max_values; i++) {

        uint32_t key = rand();
        uint32_t val = rand();

        var[key] = val;

        lock.lock();
        map[key] = val;
        lock.unlock();
      }
    });
  }

  for (auto &worker : t) {
    worker.join();
  }

  std::atomic_thread_fence(std::memory_order_seq_cst);

  for (auto &it : map) {
    auto key = it.first;
    auto val = it.second;
    EXPECT_EQ(var[key].value(), val);
  }

}

TEST_F(StateTest, test_add_range_mt) {
  StateManager &stateManager = StateManager::instance();
  StateVariable<uint32_t, uint32_t> &var = stateManager
      .registerState<uint32_t, uint32_t>("window-content-4");

  std::vector<std::thread> t;

  std::mutex mutex;
  std::unordered_map<uint32_t, uint32_t> map;
  constexpr uint32_t num_threads = 4;
  constexpr uint32_t max_values = 2000000 / num_threads;

  for (uint32_t i = 0; i < num_threads; i++) {
    t.emplace_back([&var, &map, &mutex]() {
      std::unique_lock<std::mutex> lock(mutex, std::defer_lock);
      for (size_t i = 0; i < max_values; i++) {

        uint32_t key = rand();
        uint32_t val = rand();

        var[key] = val;

        lock.lock();
        map[key] = val;
        lock.unlock();
      }
    });
  }

  for (auto &worker : t) {
    worker.join();
  }

  std::atomic_thread_fence(std::memory_order_seq_cst);

  {
    std::unique_lock<std::mutex> lock(mutex);
    auto rangeAll = var.rangeAll();
    for (auto &it : rangeAll) {
      auto key = it.first;
      auto val = it.second;

      EXPECT_EQ(map[key], val);
    }
  }

}

struct window_metadata {
  uint64_t start;
  uint64_t end;

  explicit window_metadata(uint64_t s, uint64_t e)
      :
      start(s),
      end(e) {
  }
};

TEST_F(StateTest, test_struct) {
  StateManager &stateManager = StateManager::instance();
  StateVariable<uint32_t, window_metadata*> &var = stateManager
      .registerState<uint32_t, window_metadata*>("window-content-5");

  for (size_t i = 0; i < 8192; i++) {

    uint32_t key = rand();

    uint64_t start = rand();

    uint64_t end = start + rand();

    var[key].emplace(start, end);

    auto v = var[key].value();

    EXPECT_EQ(v->start, start);
    EXPECT_EQ(v->end, end);

  }

}

}
