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

  explicit CandlestickChart(size_t capacity, QGraphicsItem *parent = nullptr);

  void SetCapacity(size_t);
  size_t GetCapacity() const;

  void Update(const QCandlestickSet &);

 private:
  void SetAxisesMinMax();
  void CheckAxisesMinMax(const QCandlestickSet &update);
  void CheckAxisesMinMax(const QList<QCandlestickSet *> &sets);

  int m_capacity;
  QCandlestickSeries *m_series;
  Price m_min;
  Price m_max;
};

}  // namespace Charts
}  // namespace FrontEnd
}  // namespace trdk