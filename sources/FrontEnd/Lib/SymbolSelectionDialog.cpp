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
#include "Engine.hpp"
#include "ui_SymbolSelectionDialog.h"

using namespace trdk::Lib;
using namespace trdk::FrontEnd::Lib;

SymbolSelectionDialog::SymbolSelectionDialog(Engine &engine, QWidget *parent)
    : Base(parent), m_ui(boost::make_unique<Ui::SymbolSelectionDialog>()) {
  m_ui->setupUi(this);
  {
    const IniFile conf(engine.GetConfigFilePath());
    const IniSectionRef defaults(conf, "Defaults");
    for (const std::string &symbol :
         defaults.ReadList("symbol_list", ",", true)) {
      m_ui->symbols->addItem(QString::fromStdString(symbol));
    }
  }

  Verify(connect(m_ui->symbols, &QListWidget::doubleClicked,
                 [this](const QModelIndex &) { accept(); }));
}

SymbolSelectionDialog::~SymbolSelectionDialog() = default;

std::vector<QString> SymbolSelectionDialog::RequestSymbols() {
  for (;;) {
    std::vector<QString> result;
    if (exec() != QDialog::Accepted) {
      return result;
    }
    if (!m_ui->symbols->selectedItems().size()) {
      QMessageBox::warning(this, tr("Symbol is not set"),
                           tr("Please select one or more symbols."),
                           QMessageBox::Ok);
      continue;
    }
    for (const auto &item : m_ui->symbols->selectedItems()) {
      result.emplace_back(item->text());
    }
    return result;
  }
}
