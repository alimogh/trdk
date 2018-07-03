/*******************************************************************************
 *   Created: 2018/01/31 23:55:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "LegSetSelectionDialog.hpp"

using namespace trdk::Lib;
using namespace trdk::FrontEnd;
using namespace trdk::Strategies::TriangularArbitrage;

LegSetSelectionDialog::LegSetSelectionDialog(Engine &, QWidget *parent)
    : QDialog(parent) {
  m_ui.setupUi(this);
  {
    m_legSets.emplace_back(LegsConf{LegConf{"ETH_BTC", OrderSide::Buy},
                                    LegConf{"BTC_USD", OrderSide::Buy},
                                    LegConf{"ETH_USD", OrderSide::Sell}});
    m_legSets.emplace_back(LegsConf{LegConf{"ETH_BTC", OrderSide::Sell},
                                    LegConf{"BTC_USD", OrderSide::Sell},
                                    LegConf{"ETH_USD", OrderSide::Buy}});
    m_legSets.emplace_back(LegsConf{LegConf{"BCH_BTC", OrderSide::Buy},
                                    LegConf{"BTC_USD", OrderSide::Buy},
                                    LegConf{"BCH_USD", OrderSide::Sell}});
    m_legSets.emplace_back(LegsConf{LegConf{"BCH_BTC", OrderSide::Sell},
                                    LegConf{"BTC_USD", OrderSide::Sell},
                                    LegConf{"BCH_USD", OrderSide::Buy}});
    m_legSets.emplace_back(LegsConf{LegConf{"BCH_ETH", OrderSide::Buy},
                                    LegConf{"ETH_BTC", OrderSide::Buy},
                                    LegConf{"BCH_BTC", OrderSide::Sell}});
    m_legSets.emplace_back(LegsConf{LegConf{"BCH_ETH", OrderSide::Sell},
                                    LegConf{"ETH_BTC", OrderSide::Sell},
                                    LegConf{"BCH_BTC", OrderSide::Buy}});
    m_legSets.emplace_back(LegsConf{LegConf{"ETH_BTC", OrderSide::Buy},
                                    LegConf{"BTC_EUR", OrderSide::Buy},
                                    LegConf{"ETH_EUR", OrderSide::Sell}});
    m_legSets.emplace_back(LegsConf{LegConf{"ETH_BTC", OrderSide::Sell},
                                    LegConf{"BTC_EUR", OrderSide::Sell},
                                    LegConf{"ETH_EUR", OrderSide::Buy}});
    m_legSets.emplace_back(LegsConf{LegConf{"BCH_BTC", OrderSide::Buy},
                                    LegConf{"BTC_EUR", OrderSide::Buy},
                                    LegConf{"BCH_EUR", OrderSide::Sell}});
    m_legSets.emplace_back(LegsConf{LegConf{"BCH_BTC", OrderSide::Sell},
                                    LegConf{"BTC_EUR", OrderSide::Sell},
                                    LegConf{"BCH_EUR", OrderSide::Buy}});
  }
  for (const auto &legs : m_legSets) {
    QStringList subs;
    const auto &append = [&subs](const LegConf &leg) {
      subs << QString("%1 %2").arg(ConvertToUiString(leg.side), leg.symbol);
    };
    for (const auto &leg : legs) {
      append(leg);
    }
    m_ui.legSets->addItem(subs.join(", "));
  }

  Verify(connect(m_ui.legSets, &QListWidget::doubleClicked,
                 [this](const QModelIndex &) { accept(); }));
}

std::vector<LegsConf> LegSetSelectionDialog::RequestLegSet() {
  for (;;) {
    std::vector<LegsConf> result;
    if (exec() != QDialog::Accepted) {
      return result;
    }
    if (m_ui.legSets->selectedItems().isEmpty()) {
      QMessageBox::warning(this, tr("Symbol is not set"),
                           tr("Please select one or more symbol sets."),
                           QMessageBox::Ok);
      continue;
    }
    for (auto &index : m_ui.legSets->selectionModel()->selectedIndexes()) {
      AssertLe(0, index.row());
      AssertGt(m_legSets.size(), index.row());
      result.emplace_back(m_legSets[index.row()]);
    }
    return result;
  }
}
