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
namespace Charts {

class TRDK_FRONTEND_CHARTS_API ChartToolbarWidget : public QWidget {
  Q_OBJECT

 public:
  typedef QWidget Base;

  explicit ChartToolbarWidget(QWidget *parent = nullptr);
  ~ChartToolbarWidget();

  size_t GetNumberOfSecondsInFrame() const;

 signals:
  void NumberOfSecondsInFrameChange(size_t);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace Charts
}  // namespace FrontEnd
}  // namespace trdk