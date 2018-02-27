/*******************************************************************************
 *   Created: 2018/02/27 09:54:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "RelativeStrengthIndex.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::Accumulators;
using namespace trdk::TradingLib;

namespace accs = boost::accumulators;

class RelativeStrengthIndex::Implementation {
 public:
  const size_t m_numberOfPeriods;

  boost::optional<Double> m_prevValue;
  accs::accumulator_set<
      Double,
      accs::stats<accs::tag::MovingAverageSmoothed, accs::tag::count>>
      m_maU;
  MovingAverage::Smoothed m_maD;

  boost::optional<Double> m_lastValue;

  explicit Implementation(size_t numberOfPeriods)
      : m_numberOfPeriods(numberOfPeriods),
        m_maU(accs::tag::rolling_window::window_size = m_numberOfPeriods),
        m_maD(accs::tag::rolling_window::window_size = m_numberOfPeriods) {}

  void Append(const Double &value) {
    if (!m_prevValue) {
      m_prevValue = value;
      return;
    }

    {
      const Double diff = value - *m_prevValue;
      if (diff > 0) {
        m_maU(diff);
        m_maD(0);
      } else if (diff < 0) {
        m_maU(0);
        m_maD(-diff);
      } else {
        m_maU(0);
        m_maD(0);
      }
      m_prevValue = value;
    }

    if (accs::count(m_maU) < m_numberOfPeriods) {
      return;
    }

    Double result;
    const Double maD = accs::smma(m_maD);
    if (maD == 0) {
      result = 100;
    } else {
      const auto rs = accs::smma(m_maU) / maD;
      result = 100.0 - (100.0 / (1.0 + rs));
    }

    m_lastValue.emplace(std::move(result));
  }
};

RelativeStrengthIndex::RelativeStrengthIndex(size_t numberOfPeriods)
    : m_pimpl(boost::make_unique<Implementation>(numberOfPeriods)) {}

RelativeStrengthIndex::RelativeStrengthIndex(const RelativeStrengthIndex &rhs)
    : m_pimpl(boost::make_unique<Implementation>(*rhs.m_pimpl)) {}

RelativeStrengthIndex::~RelativeStrengthIndex() = default;

RelativeStrengthIndex::RelativeStrengthIndex(RelativeStrengthIndex &&) =
    default;

RelativeStrengthIndex &RelativeStrengthIndex::operator=(
    const RelativeStrengthIndex &rhs) {
  RelativeStrengthIndex(rhs).Swap(*this);
  return *this;
}

void RelativeStrengthIndex::Swap(RelativeStrengthIndex &rhs) noexcept {
  m_pimpl.swap(rhs.m_pimpl);
}

void RelativeStrengthIndex::Append(const Double &value) {
  return m_pimpl->Append(value);
}

size_t RelativeStrengthIndex::GetNumberOfPeriods() const {
  return m_pimpl->m_numberOfPeriods;
}

bool RelativeStrengthIndex::IsFull() const {
  return m_pimpl->m_lastValue ? true : false;
}

const Double &RelativeStrengthIndex::GetLastValue() const {
  Assert(IsFull());
  return m_pimpl->m_lastValue.get_value_or(0);
}
