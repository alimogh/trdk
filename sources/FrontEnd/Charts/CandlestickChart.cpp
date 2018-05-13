//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "CandlestickChart.hpp"

using namespace trdk::Lib;
using namespace trdk::FrontEnd::Charts;

CandlestickChart::CandlestickChart(const size_t capacity, QGraphicsItem *parent)
    : Base(parent),
      m_capacity(static_cast<decltype(m_capacity)>(capacity)),
      m_min(0),
      m_max(0) {
  {
    auto series = boost::make_unique<QCandlestickSeries>();
    addSeries(&*series);
    m_series = series.release();
  }
  m_series->setIncreasingColor(QColor(Qt::green));
  m_series->setDecreasingColor(QColor(Qt::red));
  m_series->setBodyOutlineVisible(false);
  createDefaultAxes();
  setAnimationOptions(NoAnimation);
  {
    QStringList categories;
    for (auto i = 1; i <= m_capacity; ++i) {
      categories.push_back(QString::number(i));
    }
    auto &axis = *qobject_cast<QBarCategoryAxis *>(axes(Qt::Horizontal).at(0));
    axis.setCategories(categories);
    axis.setLabelsVisible(false);
    axis.setGridLineVisible(false);
  }
  {
    auto &axis = *qobject_cast<QValueAxis *>(axes(Qt::Vertical).at(0));
    axis.setGridLineVisible(true);
    axis.setRange(0, 0);
  }
}

void CandlestickChart::SetCapacity(const size_t capacity) {
  m_capacity = static_cast<decltype(m_capacity)>(capacity);
  if (m_series->count() <= m_capacity) {
    return;
  }
  auto sets = m_series->sets();
  while (sets.size() >= m_capacity) {
    m_series->remove(sets.front());
    sets.pop_front();
  }
  CheckAxisesMinMax(sets);
}

size_t CandlestickChart::GetCapacity() const {
  return static_cast<size_t>(m_capacity);
}

void CandlestickChart::Update(const QCandlestickSet &update) {
  auto sets = m_series->sets();

  if (!sets.empty()) {
    auto &set = *sets.back();
    if (static_cast<uintmax_t>(set.timestamp()) ==
        static_cast<uintmax_t>(update.timestamp())) {
      AssertEq(Price(set.open()), update.open());
      AssertLe(Price(set.high()), update.high());
      AssertGe(Price(set.low()), update.low());
      CheckAxisesMinMax(update);
      set.setHigh(update.high());
      set.setLow(update.low());
      set.setClose(update.close());
      AssertLe(Price(set.open()), set.high());
      AssertGe(Price(set.open()), set.low());
      AssertLe(Price(set.close()), set.high());
      AssertGe(Price(set.close()), set.low());
      AssertLe(Price(set.low()), set.high());
      return;
    }
  }

  while (sets.size() >= m_capacity) {
    m_series->remove(sets.front());
    sets.pop_front();
  }
  {
    auto newSet = std::make_unique<QCandlestickSet>(
        update.open(), update.high(), update.low(), update.close(),
        update.timestamp(), update.parent());
    AssertLe(Price(newSet->open()), newSet->high());
    AssertGe(Price(newSet->open()), newSet->low());
    AssertLe(Price(newSet->close()), newSet->high());
    AssertGe(Price(newSet->close()), newSet->low());
    AssertLe(Price(newSet->low()), newSet->high());
    sets.push_back(&*newSet);
    CheckAxisesMinMax(sets);
    m_series->append(&*newSet);
    newSet.release();
  }
}

void CandlestickChart::CheckAxisesMinMax(const QCandlestickSet &update) {
  auto isMinMaxChanged = false;
  if (m_min == 0 || m_min > update.low()) {
    m_min = update.low();
    isMinMaxChanged = true;
  }
  if (m_max < update.high()) {
    m_max = update.high();
    isMinMaxChanged = true;
  }
  if (!isMinMaxChanged) {
    return;
  }
  SetAxisesMinMax();
}

void CandlestickChart::CheckAxisesMinMax(const QList<QCandlestickSet *> &sets) {
  m_min = m_max = 0;
  for (const auto &set : sets) {
    if (m_min == 0 || m_min > set->low()) {
      m_min = set->low();
    }
    if (m_max < set->high()) {
      m_max = set->high();
    }
  }
  if (!m_min && !m_max) {
    return;
  }
  SetAxisesMinMax();
}

void CandlestickChart::SetAxisesMinMax() {
  AssertLe(m_min, m_max);
  const auto margin = 0;
  qobject_cast<QValueAxis *>(axes(Qt::Vertical).at(0))
      ->setRange(m_min - margin, m_max + margin);
}
