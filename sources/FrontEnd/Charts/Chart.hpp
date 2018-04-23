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

namespace trdk {
namespace FrontEnd {
namespace Charts {

class Chart : public QChart {
 public:
  typedef QChart Base;

  explicit Chart(QGraphicsItem *parent = nullptr);
};

}  // namespace Charts
}  // namespace FrontEnd
}  // namespace trdk