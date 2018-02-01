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
#include "SymbolSelectionDialog.hpp"

using namespace trdk::Lib;
using namespace trdk::Strategies::MarketMaker;
using namespace trdk::FrontEnd::Lib;

SymbolSelectionDialog::SymbolSelectionDialog(Engine &engine, QWidget *parent)
    : Base(parent) {
  m_ui.setupUi(this);
  {
    const IniFile conf(engine.GetConfigFilePath());
    const IniSectionRef defaults(conf, "Defaults");
    for (const std::string &symbol :
         defaults.ReadList("symbol_list", ",", true)) {
      m_ui.symbol->addItem(QString::fromStdString(symbol));
    }
  }
}

boost::optional<QString> SymbolSelectionDialog::RequestSymbol() {
  for (;;) {
    if (exec() != QDialog::Accepted) {
      return boost::none;
    }
    if (!m_ui.symbol->selectedItems().size()) {
      QMessageBox::warning(this, tr("Symbol is not set"),
                           tr("Please select a symbol."), QMessageBox::Ok);
      continue;
    }
    return m_ui.symbol->selectedItems().first()->text();
  }
}