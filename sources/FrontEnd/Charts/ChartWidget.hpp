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

#include "Api.h"
#include "Fwd.hpp"

namespace trdk {
namespace FrontEnd {
namespace Charts {

class TRDK_FRONTEND_CHARTS_API ChartWidget : public QWidget {
  Q_OBJECT

 public:
  typedef QWidget Base;

  explicit ChartWidget(QWidget *parent);
  virtual ~ChartWidget();

 public slots:
  virtual void OnPriceUpdate(const QDateTime &, const Price &) = 0;

 protected:
  ChartView &GetView();
  const ChartView &GetView() const;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace Charts
}  // namespace FrontEnd
}  // namespace trdk