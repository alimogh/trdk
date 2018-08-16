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
using namespace trdk::FrontEnd;

SymbolSelectionDialog::SymbolSelectionDialog(Engine &engine, QWidget *parent)
    : Base(parent), m_ui(boost::make_unique<Ui::SymbolSelectionDialog>()) {
  for (const auto &symbol : engine.GetContext().GetSymbolListHint()) {
    m_symbols.emplace_back(QString::fromStdString(symbol));
  }

  m_ui->setupUi(this);

  UpdateList();

  Verify(connect(m_ui->filter, &QLineEdit::textEdited,
                 [this]() { UpdateList(); }));
  Verify(connect(m_ui->symbols, &QListWidget::doubleClicked,
                 [this](const QModelIndex &) { accept(); }));
}

SymbolSelectionDialog::~SymbolSelectionDialog() = default;

std::vector<QString> SymbolSelectionDialog::RequestSymbols() {
  for (;;) {
    std::vector<QString> result;
    if (exec() != Accepted) {
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

void SymbolSelectionDialog::UpdateList() {
  const QSignalBlocker blocker(m_ui->symbols);

  m_ui->symbols->clear();

  const auto &filter = m_ui->filter->text();
  for (const auto &symbol : m_symbols) {
    if (!symbol.contains(filter, Qt::CaseInsensitive)) {
      continue;
    }
    m_ui->symbols->addItem(symbol);
  }
}
