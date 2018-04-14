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

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API TotalResultsReportView : public QTableView {
  Q_OBJECT

 public:
  typedef QTableView Base;

  explicit TotalResultsReportView(QWidget *parent);
};
}  // namespace FrontEnd
}  // namespace trdk