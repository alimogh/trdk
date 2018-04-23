//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Api.h"
#include "Lib/Fwd.hpp"

namespace trdk {
namespace FrontEnd {
namespace Charts {

class TRDK_FRONTEND_CHARTS_API ChartWidget : public QWidget {
  Q_OBJECT

 public:
  typedef QWidget Base;

  explicit ChartWidget(QWidget *parent);

 public slots:
  void OnPriceUpdate(const QDateTime &, const Price &);
};

}  // namespace Charts
}  // namespace FrontEnd
}  // namespace trdk