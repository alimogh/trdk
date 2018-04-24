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

class ChartWidget::Implementation : boost::noncopyable {
 public:
  ChartView m_view;

  explicit Implementation(ChartWidget& self) : m_view(&self) {}
};

ChartWidget::ChartWidget(QWidget* parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>(*this)) {
  {
    auto& layout = *new QVBoxLayout(this);
    setLayout(&layout);
    layout.addWidget(&GetView());
  }
}

ChartWidget::~ChartWidget() = default;

ChartView& ChartWidget::GetView() { return m_pimpl->m_view; }

const ChartView& ChartWidget::GetView() const {
  return const_cast<ChartWidget*>(this)->GetView();
}
