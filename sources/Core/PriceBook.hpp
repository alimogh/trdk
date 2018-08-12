/*******************************************************************************
 *   Created: 2016/01/04 18:03:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Api.h"

namespace trdk {

class PriceBook {
 public:
  enum { SIDE_MAX_SIZE = 10 };

  class Level {
   public:
    Level() : m_price(.0), m_qty(.0) {}

    explicit Level(const boost::posix_time::ptime& time,
                   const Price& price,
                   const Qty& qty)
        : m_time(time), m_price(price), m_qty(qty) {}

    void operator+=(const Level& rhs) { m_qty += rhs.m_qty; }
    void operator+=(const Qty& qty) { m_qty += qty; }
    const boost::posix_time::ptime& GetTime() const { return m_time; }

    void UpdateTime(const boost::posix_time::ptime& time) {
      if (m_time == boost::posix_time::not_a_date_time || time > m_time) {
        m_time = time;
      }
    }

    const Price& GetPrice() const { return m_price; }

    const Qty& GetQty() const { return m_qty; }

   private:
    boost::posix_time::ptime m_time;

    Price m_price;
    Qty m_qty;
  };

  template <bool isAscendingSort>
  class Side {
   private:
    typedef boost::array<Level, SIDE_MAX_SIZE> Storage;

   public:
    Side() : m_offset(0), m_size(0) {}

    Side(const Side& rhs) : m_offset(0), m_size(rhs.m_size) {
      const auto& begin = rhs.m_levels.cbegin() + rhs.m_offset;
      const auto& end = begin + m_size;
      AssertGe(m_levels.size(), size_t(std::distance(begin, end)));
      std::copy(begin, end, m_levels.begin());
    }

    const Side& operator=(const Side& rhs) {
      const auto& begin = rhs.m_levels.cbegin() + rhs.m_offset;
      const auto& end = begin + rhs.m_size;
      AssertGe(m_levels.size(), size_t(std::distance(begin, end)));
      std::copy(begin, end, m_levels.begin());
      m_offset = 0;
      m_size = rhs.m_size;
      return *this;
    }

   public:
    size_t GetSize() const { return m_size; }

    bool IsEmpty() const { return GetSize() == 0; }

    const Level& GetTop() const { return GetLevel(0); }

    const Level& GetLevel(size_t levelIndex) const {
      AssertGe(m_levels.size(), size_t(m_offset + m_size));
      if (levelIndex >= m_size) {
        throw Lib::LogicError("Price book level index is out of range");
      }
      AssertGt(m_levels.size(), m_offset + levelIndex);
      return m_levels[m_offset + levelIndex];
    }

    void PopTop() {
      AssertGe(m_levels.size(), size_t(m_offset + m_size));
      if (IsEmpty()) {
        throw Lib::LogicError("Price book is empty");
      }
      AssertGt(m_levels.size(), m_offset);
      AssertLt(0, m_size);

      ++m_offset;
      --m_size;

      AssertGe(m_levels.size(), size_t(m_offset + m_size));
    }

    void Add(const boost::posix_time::ptime& time,
             double price,
             const Qty& qty) {
      AssertEq(0, m_offset);
      AssertGe(m_levels.size(), m_size);

      if (m_size >= m_levels.size()) {
        AssertEq(m_size, m_levels.size());
        throw Lib::Exception("Price book is out of price levels slots");
      }

      const auto& begin = m_levels.begin();
      const auto& end = begin + m_size;
      const auto& pos = FindPrice(begin, end, price);
      TrdkAssert(pos <= end);
      TrdkAssert(pos < m_levels.cend());
      TrdkAssert(pos >= begin);

      if (pos == end) {
        *pos = Level(time, price, qty);
        ++m_size;
        return;
      }

      if (pos->GetPrice() == price) {
        throw Lib::Exception("Not unique price level found");
      }

      for (auto it = end; it != pos; --it) {
        *it = *std::prev(it);
      }
      *pos = Level(time, price, qty);
      ++m_size;
    }

    bool Update(const boost::posix_time::ptime& time,
                double price,
                const Qty& qty) {
      AssertEq(0, m_offset);
      AssertGe(m_levels.size(), m_size);

      const auto& begin = m_levels.begin();
      const auto& end = begin + m_size;
      const auto& pos = FindPrice(begin, end, price);
      TrdkAssert(pos <= end);
      TrdkAssert(pos >= begin);

      if (pos == end) {
        if (m_size >= m_levels.size()) {
          AssertEq(m_levels.size(), m_size);
          AssertEq(m_levels.end(), end);
          return false;
        }

        TrdkAssert(end < m_levels.cend());
        *pos = Level(time, price, qty);
        ++m_size;
        return true;
      }

      if (pos->GetPrice() == price) {
        Level& level = *pos;
        level += qty;
        level.UpdateTime(time);
        return true;
      }

      Storage::iterator it;
      if (m_size < m_levels.size()) {
        TrdkAssert(end < m_levels.cend());
        it = end;
        ++m_size;
      } else {
        AssertEq(m_levels.size(), m_size);
        TrdkAssert(end == m_levels.cend());
        it = std::prev(end);
      }
      for (; it != pos; --it) {
        *it = *std::prev(it);
      }

      *pos = Level(time, price, qty);

      return true;
    }

    void Clear() throw() { m_offset = m_size = 0; }

   private:
    static Storage::iterator FindPrice(const Storage::iterator& begin,
                                       const Storage::iterator& end,
                                       const Price& price) {
      static_assert(isAscendingSort, "Failed to find template specialization.");
      return std::lower_bound(begin, end, price,
                              [](const Level& lhs, const Price& rhs) {
                                return lhs.GetPrice() < rhs;
                              });
    }

   private:
    uint8_t m_offset;
    uint8_t m_size;
    Storage m_levels;
  };

  typedef Side<false> Bid;
  typedef Side<true> Ask;

 public:
  PriceBook() {}

  explicit PriceBook(const boost::posix_time::ptime& time) : m_time(time) {}

 public:
  static size_t GetSideMaxSize() { return SIDE_MAX_SIZE; }

 public:
  const boost::posix_time::ptime& GetTime() const { return m_time; }

  void SetTime(const boost::posix_time::ptime& time) {
    TrdkAssert(m_time == boost::posix_time::not_a_date_time || m_time <= time);
    m_time = time;
  }

  const Bid& GetBid() const { return const_cast<PriceBook*>(this)->GetBid(); }
  Bid& GetBid() { return m_bid; }

  const Ask& GetAsk() const { return const_cast<PriceBook*>(this)->GetAsk(); }
  Ask& GetAsk() { return m_ask; }

  void Clear() throw() {
    m_bid.Clear();
    m_ask.Clear();
  }

 private:
  boost::posix_time::ptime m_time;

  Bid m_bid;
  Ask m_ask;
};

template <>
inline PriceBook::Side<false>::Storage::iterator
PriceBook::Side<false>::FindPrice(const Storage::iterator& begin,
                                  const Storage::iterator& end,
                                  const Price& price) {
  return std::lower_bound(
      begin, end, price,
      [](const Level& lhs, const Price& rhs) { return lhs.GetPrice() > rhs; });
}
}  // namespace trdk
