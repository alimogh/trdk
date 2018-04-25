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
  std::unique_ptr<CandlestickChart> m_chart;

  QDateTime m_setTime;
  QCandlestickSet m_set;

  size_t m_numberOfSecondsInFrame;

  explicit Implemnetation(CandlestickChartWidget& self,
                          const size_t numberOfSecondsInFrame,
                          const size_t capacity)
      : m_chart(std::make_unique<CandlestickChart>(capacity)),
        m_set(0, &self),
        m_numberOfSecondsInFrame(numberOfSecondsInFrame) {
    AssertEq(0, m_numberOfSecondsInFrame % 60);
  }

  void UpdatePrice(const QDateTime& updateTime, const Price& price) {
    if (!m_setTime.isValid() ||
        m_setTime.addSecs(m_numberOfSecondsInFrame) <= updateTime) {
      m_setTime = updateTime;
      const auto& time = m_setTime.time();
      m_setTime.setTime(
          {time.hour(),
           static_cast<int>((time.minute() / (m_numberOfSecondsInFrame / 60)) *
                            (m_numberOfSecondsInFrame / 60)),
           0});
      m_set.setTimestamp(static_cast<qreal>(m_setTime.toSecsSinceEpoch()));
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
    m_chart->Update(m_set);
  }
};

CandlestickChartWidget::CandlestickChartWidget(
    const size_t numberOfSecondsInFrame, const size_t capacity, QWidget* parent)
    : Base(parent),
      m_pimpl(boost::make_unique<Implemnetation>(
          *this, numberOfSecondsInFrame, capacity)) {
  GetView().setChart(&*m_pimpl->m_chart);
}

CandlestickChartWidget::~CandlestickChartWidget() = default;

void CandlestickChartWidget::SetNumberOfSecondsInFrame(
    const size_t numberOfSecondsInFrame) {
  if (m_pimpl->m_numberOfSecondsInFrame != numberOfSecondsInFrame) {
    return;
  }
  const auto capacity = GetCapacity();
  m_pimpl.reset();
  m_pimpl = boost::make_unique<Implemnetation>(*this, numberOfSecondsInFrame,
                                               capacity);
  GetView().setChart(&*m_pimpl->m_chart);
}

size_t CandlestickChartWidget::GetNumberOfSecondsInFrame() const {
  return m_pimpl->m_numberOfSecondsInFrame;
}

void CandlestickChartWidget::SetCapacity(const size_t capacity) {
  m_pimpl->m_chart->SetCapacity(capacity);
}

size_t CandlestickChartWidget::GetCapacity() const {
  return m_pimpl->m_chart->GetCapacity();
}

void CandlestickChartWidget::UpdatePrice(const QDateTime& time,
                                         const Price& price) {
  m_pimpl->UpdatePrice(time, price);
}
