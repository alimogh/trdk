//
//    Created: 2018/06/14 19:17
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

class TRDK_FRONTEND_LIB_API SecurityListView : public QTableView {
  Q_OBJECT

 public:
  typedef QTableView Base;

  explicit SecurityListView(QWidget *parent);
  ~SecurityListView();

 signals:
  void ChartRequested(const QString &symbol);
  void OrderRequested(Security &);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace FrontEnd
}  // namespace trdk
