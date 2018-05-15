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
namespace pt = boost::posix_time;

class CandlestickChartWidget::Implemnetation : boost::noncopyable {
 public:
  CandlestickChartWidget& m_self;
  CandlestickChart* m_chart;

  size_t m_numberOfSecondsInFrame;

  explicit Implemnetation(CandlestickChartWidget& self,
                          const size_t numberOfSecondsInFrame)
      : m_self(self), m_numberOfSecondsInFrame(numberOfSecondsInFrame) {
    AssertEq(0, m_numberOfSecondsInFrame % 60);
  }

  void Update(const Bar& bar) {
    m_chart->Update(QCandlestickSet{
        *bar.openPrice, *bar.highPrice, *bar.lowPrice, *bar.closePrice,
                        static_cast<qreal>(pt::to_time_t(bar.time)), &m_self});
  }
};

CandlestickChartWidget::CandlestickChartWidget(
    const size_t numberOfSecondsInFrame, const size_t capacity, QWidget* parent)
    : Base(parent),
      m_pimpl(
          boost::make_unique<Implemnetation>(*this, numberOfSecondsInFrame)) {
  auto chart = std::make_unique<CandlestickChart>(capacity);
  m_pimpl->m_chart = &*chart;
  GetView().setChart(m_pimpl->m_chart);
  chart.release();
}

CandlestickChartWidget::~CandlestickChartWidget() = default;

void CandlestickChartWidget::SetNumberOfSecondsInFrame(
    const size_t numberOfSecondsInFrame) {
  if (m_pimpl->m_numberOfSecondsInFrame == numberOfSecondsInFrame) {
    return;
  }
  const auto capacity = GetCapacity();
  m_pimpl = boost::make_unique<Implemnetation>(*this, numberOfSecondsInFrame);
  auto chart = std::make_unique<CandlestickChart>(capacity);
  m_pimpl->m_chart = &*chart;
  GetView().setChart(m_pimpl->m_chart);
  chart.release();
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

void CandlestickChartWidget::Update(const Bar& bar) { m_pimpl->Update(bar); }
