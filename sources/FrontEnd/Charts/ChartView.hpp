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

#include "Chart.hpp"

namespace trdk {
namespace FrontEnd {
namespace Charts {

class ChartView : public QChartView {
  Q_OBJECT

 public:
  typedef QChartView Base;

  explicit ChartView(QWidget *parent);

 private:
  Chart m_chart;
};

}  // namespace Charts
}  // namespace FrontEnd
}  // namespace trdk