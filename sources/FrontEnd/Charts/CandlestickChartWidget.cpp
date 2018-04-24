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
#include "CandlestickChartWidget.hpp"
#include "CandlestickChart.hpp"
#include "ChartView.hpp"

using namespace trdk;
using namespace FrontEnd::Charts;

class CandlestickChartWidget::Implemnetation : boost::noncopyable {
 public:
  CandlestickChart m_chart;

  QDateTime m_setTime;
  QCandlestickSet m_set;

  explicit Implemnetation(CandlestickChartWidget& self) : m_set(0, &self) {}

  void OnPriceUpdate(const QDateTime& updateTime, const Price& price) {
    const auto periodMinutes = 1;
    QDateTime periodTime;
    if (!m_setTime.isValid()) {
      m_setTime = updateTime;
      m_setTime.setTime(
          {periodTime.time().hour(),
           static_cast<int>(periodTime.time().minute() / periodMinutes) *
               periodMinutes,
           0});
      periodTime = m_setTime;
    } else {
      periodTime = m_setTime;
      periodTime.addSecs(periodMinutes * 60);
      if (periodTime > updateTime) {
        periodTime = m_setTime;
      } else {
        periodTime.setTime(
            {updateTime.time().hour(),
             static_cast<int>(updateTime.time().minute() / periodMinutes) *
                 periodMinutes,
             0});
      }
    }

    const auto updateTimestamp =
        static_cast<uintmax_t>(periodTime.toSecsSinceEpoch());
    const auto setTimestamp =
        static_cast<decltype(updateTimestamp)>(m_set.timestamp());
    if (setTimestamp != updateTimestamp) {
      AssertLt(setTimestamp, updateTimestamp);
      m_set.setTimestamp(static_cast<qreal>(updateTimestamp));
      m_set.setOpen(price);
      m_set.setHigh(price);
      m_set.setLow(price);
      m_set.setClose(price);
    } else {
      if (m_set.high() < price) {
        m_set.setHigh(price);
      } else if (m_set.low() > price) {
        m_set.setLow(price);
      }
      m_set.setClose(price);
    }
    m_chart.Update(m_set);
  }
};

CandlestickChartWidget::CandlestickChartWidget(QWidget* parent)
    : Base(parent), m_pimpl(boost::make_unique<Implemnetation>(*this)) {
  GetView().setChart(&m_pimpl->m_chart);
}

CandlestickChartWidget::~CandlestickChartWidget() = default;

void CandlestickChartWidget::OnPriceUpdate(const QDateTime& time,
                                           const Price& price) {
  m_pimpl->OnPriceUpdate(time, price);
}
