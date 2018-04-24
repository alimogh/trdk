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

class CandlestickChart : public Chart {
 public:
  typedef Chart Base;

  explicit CandlestickChart(QGraphicsItem *parent = nullptr);

  void Update(const QCandlestickSet &);

 private:
  Price m_min;
  Price m_max;
  QStringList m_categories;
  QCandlestickSeries *m_series;
};

}  // namespace Charts
}  // namespace FrontEnd
}  // namespace trdk