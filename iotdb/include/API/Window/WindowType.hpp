#ifndef IOTDB_INCLUDE_API_WINDOW_WINDOWTYPE_HPP_
#define IOTDB_INCLUDE_API_WINDOW_WINDOWTYPE_HPP_

#include <API/AbstractWindowDefinition.hpp>
#include <API/Window/WindowMeasure.hpp>

namespace iotdb {

/**
 * A TumblingWindow assigns records to non-overlapping windows.
 */
class TumblingWindow : public WindowType {
 public:
  /**
   * Creates a new TumblingWindow that assigns
   * elements to time windows based on the element timestamp and offset.
   * For example, if you want window a stream by hour,but window begins at the 15th minutes
   * of each hour, you can use {@code of(Time.hours(1),Time.minutes(15))},then you will get
   * time windows start at 0:15:00,1:15:00,2:15:00,etc.
   * @param size
   * @return WindowTypePtr
   */
  static WindowTypePtr of(TimeMeasure size);

  /**
   * Calculates the next window end based on a given timestamp.
   * @param currentTs
   * @return the next window end
   */
  uint64_t calculateNextWindowEnd(uint64_t currentTs) const override {
    return currentTs + _size.getTime() - (currentTs) % _size.getTime();
  }

 private:
  TumblingWindow(TimeMeasure size);
  const TimeMeasure _size;
};

/**
 * A SlidingWindow assigns records to multiple overlapping windows.
 * TODO This is currently not implemented!
 */
class SlidingWindow : public WindowType {
 public:
  static WindowTypePtr of(TimeMeasure size, TimeMeasure slide);
  /**
  * Calculates the next window end based on a given timestamp.
  * @param currentTs
  * @return the next window end
  */
  uint64_t calculateNextWindowEnd(uint64_t currentTs) const override {
    return 0;
  }
 private:
  SlidingWindow(TimeMeasure size, TimeMeasure slide);

  const TimeMeasure _size;
  const TimeMeasure _slide;
};

/**
 * A SessionWindow assigns records into sessions based on the timestamp of the
 * elements. Windows cannot overlap.
 * TODO This is currently not implemented!
 */
class SessionWindow : public WindowType {
 public:
  /**
  * Creates a new SessionWindow that assigns
  * elements to sessions based on the element timestamp.
  * @param size The session timeout, i.e. the time gap between sessions
  */
  static WindowTypePtr withGap(TimeMeasure gap);
  /**
  * Calculates the next window end based on a given timestamp.
  * @param currentTs
  * @return the next window end
  */
  uint64_t calculateNextWindowEnd(uint64_t currentTs) const override {
    return 0;
  }
 private:
  SessionWindow(TimeMeasure gap);
  const TimeMeasure _gap;
};
}

#endif //IOTDB_INCLUDE_API_WINDOW_WINDOWTYPE_HPP_
