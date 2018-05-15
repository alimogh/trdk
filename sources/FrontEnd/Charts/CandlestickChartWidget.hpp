//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "ChartWidget.hpp"

namespace trdk {
namespace FrontEnd {
namespace Charts {

class TRDK_FRONTEND_CHARTS_API CandlestickChartWidget : public ChartWidget {
  Q_OBJECT

 public:
  typedef ChartWidget Base;

  explicit CandlestickChartWidget(size_t numberOfSecondsInFrame,
                                  size_t capacity,
                                  QWidget* parent);
  ~CandlestickChartWidget() override;

  size_t GetCapacity() const;
  size_t GetNumberOfSecondsInFrame() const;

 public slots:
  void SetCapacity(size_t);
  void SetNumberOfSecondsInFrame(size_t);
  void Update(const Bar&) override;

 private:
  class Implemnetation;
  std::unique_ptr<Implemnetation> m_pimpl;
};

}  // namespace Charts
}  // namespace FrontEnd
}  // namespace trdk