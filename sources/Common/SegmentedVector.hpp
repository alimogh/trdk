/**************************************************************************
 *   Created: 2013/10/28 13:40:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include <boost/shared_ptr.hpp>
#include <vector>

namespace trdk {
namespace Lib {

template <typename T, size_t segmentSize>
class SegmentedVector {
 public:
  typedef T ValueType;
  enum { SEGMENT_SIZE = segmentSize };

 private:
  typedef std::vector<ValueType> Segment;

  class SegmentAllocator : public std::allocator<Segment> {
    size_t max_size() const { return SEGMENT_SIZE; }
  };

  typedef std::vector<boost::shared_ptr<Segment>, SegmentAllocator> Storage;

 public:
  SegmentedVector() : m_size(0) {}

  bool IsEmpty() const { return m_size == 0; }

  size_t GetSize() const { return m_size; }

  //! Returns element by index.
  /** If the container size is greater than n, the function never
    * throws exceptions (no-throw guarantee). Otherwise, the behavior
    * is undefined.
    */
  ValueType &operator[](size_t i) {
    const size_t segmentIndex = i / SEGMENT_SIZE;
    const size_t recordIndex = i % SEGMENT_SIZE;
    AssertLt(segmentIndex, m_storage.size());
    AssertLt(recordIndex, m_storage[segmentIndex]->size());
    return (*m_storage[segmentIndex])[recordIndex];
  }

  //! Returns element by index.
  /** If the container size is greater than n, the function never
    * throws exceptions (no-throw guarantee). Otherwise, the behavior
    * is undefined.
    */
  const ValueType &operator[](size_t i) const {
    return const_cast<SegmentedVector *>(this)->operator[](i);
  }

  void PushBack(const ValueType &newElement) {
    if (!m_storage.empty() && m_storage.back()->capacity() > 0) {
      m_storage.back()->push_back(newElement);
    } else {
      boost::shared_ptr<Segment> segment(new Segment);
      segment->push_back(newElement);
      m_storage.push_back(segment);
    }

    ++m_size;

#ifdef DEV_VER
    {
      size_t realSize = 0;
      foreach (const auto &i, m_storage) { realSize += i->size(); }
      AssertEq(realSize, m_size);
    }
#endif
  }

 private:
  Storage m_storage;
  size_t m_size;
};
}
}
