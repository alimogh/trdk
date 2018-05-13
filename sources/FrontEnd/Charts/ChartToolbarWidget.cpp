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
#include "ChartToolbarWidget.hpp"
#include "GeneratedFiles/ui_ChartToolbarWidget.h"

using namespace trdk::FrontEnd::Charts;

class ChartToolbarWidget::Implementation : boost::noncopyable {
 public:
  ChartToolbarWidget &m_self;

  Ui::ChartToolbarWidget m_ui;

  explicit Implementation(ChartToolbarWidget &self) : m_self(self) {}

  void ConnectFrameSize(QPushButton &button) {
    Verify(connect(&button, &QPushButton::clicked, &m_self,
                   [this, &button](bool isChecked) {
                     if (!isChecked) {
                       const QSignalBlocker blocker(button);
                       button.setChecked(true);
                       return;
                     }
                     const auto &unset = [this,
                                          &button](QPushButton &anotherButton) {
                       if (&anotherButton == &button) {
                         return;
                       }
                       const QSignalBlocker blocker(anotherButton);
                       anotherButton.setChecked(false);
                     };
                     unset(*m_ui.setFrameSize1m);
                     unset(*m_ui.setFrameSize5m);
                     unset(*m_ui.setFrameSize15m);
                     unset(*m_ui.setFrameSize1h);
                     unset(*m_ui.setFrameSize6h);
                     unset(*m_ui.setFrameSize1d);
                     emit m_self.NumberOfSecondsInFrameChange(
                         m_self.GetNumberOfSecondsInFrame());
                   }));
  }
};

ChartToolbarWidget::ChartToolbarWidget(QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>(*this)) {
  m_pimpl->m_ui.setupUi(this);
  m_pimpl->ConnectFrameSize(*m_pimpl->m_ui.setFrameSize1m);
  m_pimpl->ConnectFrameSize(*m_pimpl->m_ui.setFrameSize5m);
  m_pimpl->ConnectFrameSize(*m_pimpl->m_ui.setFrameSize15m);
  m_pimpl->ConnectFrameSize(*m_pimpl->m_ui.setFrameSize1h);
  m_pimpl->ConnectFrameSize(*m_pimpl->m_ui.setFrameSize6h);
  m_pimpl->ConnectFrameSize(*m_pimpl->m_ui.setFrameSize1d);
}

ChartToolbarWidget::~ChartToolbarWidget() = default;

size_t ChartToolbarWidget::GetNumberOfSecondsInFrame() const {
  if (m_pimpl->m_ui.setFrameSize1m->isChecked()) {
    return 60;
  }
  if (m_pimpl->m_ui.setFrameSize15m->isChecked()) {
    return 60 * 15;
  }
  if (m_pimpl->m_ui.setFrameSize1h->isChecked()) {
    return 60 * 60;
  }
  if (m_pimpl->m_ui.setFrameSize6h->isChecked()) {
    return (60 * 60) * 6;
  }
  if (m_pimpl->m_ui.setFrameSize1d->isChecked()) {
    return (60 * 60) * 24;
  }
  Assert(m_pimpl->m_ui.setFrameSize5m->isChecked());
  return 60 * 5;
}
