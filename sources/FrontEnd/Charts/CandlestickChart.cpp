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

namespace {
std::unique_ptr<QCandlestickSet> Clone(const QCandlestickSet &update) {
  return std::make_unique<QCandlestickSet>(update.open(), update.high(),
                                           update.low(), update.close(),
                                           update.timestamp(), update.parent());
}

}  // namespace

CandlestickChart::CandlestickChart(QGraphicsItem *parent)
    : Base(parent), m_min(0), m_max(0) {
  {
    auto series = boost::make_unique<QCandlestickSeries>();
    addSeries(&*series);
    m_series = series.release();
  }
  createDefaultAxes();
  setAnimationOptions(SeriesAnimations);
}

void CandlestickChart::Update(const QCandlestickSet &update) {
  auto sets = m_series->sets();
  const auto updateTimestamp = static_cast<uintmax_t>(update.timestamp());

  {
    auto updateMinMax = false;
    if (m_min == 0 || m_min > update.low()) {
      m_min = update.low();
      updateMinMax = true;
    }
    if (m_max < update.high()) {
      m_max = update.high();
      updateMinMax = true;
    }
    if (updateMinMax) {
      auto &axis = *qobject_cast<QValueAxis *>(axes(Qt::Vertical).at(0));
      axis.setMax(m_max * 1.01);
      axis.setMin(m_min * 0.99);
    }
  }

  for (auto index = 0; index < sets.size();) {
    auto &set = *sets[index];
    const auto setTimestamp = static_cast<uintmax_t>(set.timestamp());
    if (setTimestamp == updateTimestamp) {
      AssertEq(set.open(), update.open());
      set.setHigh(update.high());
      set.setLow(update.low());
      set.setClose(update.close());
      return;
    }
    AssertLt(setTimestamp, updateTimestamp);
    break;
  }

  {
    auto newSet = Clone(update);
    m_series->append(&*newSet);
    newSet.release();
  }
  {
    m_categories.push_back(QString::number(m_series->count()));
    auto &axisX = *qobject_cast<QBarCategoryAxis *>(axes(Qt::Horizontal).at(0));
    axisX.setCategories(m_categories);
  }
}
