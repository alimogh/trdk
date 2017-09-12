/*******************************************************************************
 *   Created: 2017/09/24 12:48:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "EngineWidget.hpp"
#include "SecurityWindow.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd::WilliamCarry;

EngineWidget::EngineWidget(QWidget *parent) : QWidget(parent) {
  m_ui.setupUi(this);
  InsertSecurity("EURUSD");
  InsertSecurity("USDJPY");
  InsertSecurity("AUDUSD");
  InsertSecurity("GBPUSD");
  InsertSecurity("USDCAD");
  InsertSecurity("NZDUSD");
  InsertSecurity("AUDCAD");
  InsertSecurity("AUDCHF");
  connect(m_ui.securityList, &QTableWidget::cellClicked, [this](int row, int) {
    ShowSecurityWindow(m_ui.securityList->item(row, 0)->text());
  });
}

void EngineWidget::InsertSecurity(const QString &symbol) {
  const auto index = m_ui.securityList->rowCount();
  m_ui.securityList->insertRow(index);
  m_ui.securityList->setItem(index, 0, new QTableWidgetItem(symbol));
  {
    QTableWidgetItem item("0.00000");
    item.setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_ui.securityList->setItem(index, 1, new QTableWidgetItem(item));
    m_ui.securityList->setItem(index, 2, new QTableWidgetItem(item));
    m_ui.securityList->setItem(index, 3, new QTableWidgetItem(item));
  }
  m_ui.securityList->setItem(index, 4, new QTableWidgetItem("00:00:00"));
}

void EngineWidget::ShowSecurityWindow(const QString &symbol) {
  {
    const auto &it = m_securities.find(symbol);
    if (it != m_securities.cend()) {
      it->second->activateWindow();
      it->second->showNormal();
      return;
    }
  }
  {
    SecurityWindow &window =
        *m_securities
             .emplace(symbol, boost::make_unique<SecurityWindow>(symbol, this))
             .first->second;
    connect(&window, &SecurityWindow::Closed,
            [this, symbol]() { CloseSecurityWindow(symbol); });
    window.show();
  }
}

void EngineWidget::CloseSecurityWindow(const QString &symbol) {
  const auto &it = m_securities.find(symbol);
  Assert(it != m_securities.cend());
  if (it == m_securities.cend()) {
    return;
  }
  m_securities.erase(it);
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<QWidget> CreateEngineFrontEnd(QWidget *parent) {
  return boost::make_unique<EngineWidget>(parent);
}

////////////////////////////////////////////////////////////////////////////////
