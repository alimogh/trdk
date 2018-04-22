//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "ChartView.hpp"

using namespace trdk::FrontEnd::Charts;

ChartView::ChartView(QWidget* parent) : Base(parent) {
  setRenderHint(QPainter::Antialiasing);
  setChart(&m_chart);
}
