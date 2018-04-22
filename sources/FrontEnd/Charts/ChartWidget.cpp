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
#include "ChartWidget.hpp"
#include "ChartView.hpp"

using namespace trdk;
using namespace FrontEnd;
using namespace Charts;

ChartWidget::ChartWidget(QWidget* parent) : Base(parent) {
  auto& view = *new ChartView(this);
  {
    auto& layout = *new QVBoxLayout(this);
    setLayout(&layout);
    layout.addWidget(&view);
  }
  view.setRenderHint(QPainter::Antialiasing);
}

void ChartWidget::OnPriceUpdate(const QDateTime&, const Price& price) {
  price;
  price;
}
