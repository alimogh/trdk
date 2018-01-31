/*******************************************************************************
 *   Created: 2018/01/31 23:47:33
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "ui_SymbolSelectionDialog.h"

namespace trdk {
namespace Strategies {
namespace MarketMaker {

class SymbolSelectionDialog : public QDialog {
  Q_OBJECT
 public:
  typedef QDialog Base;

 public:
  explicit SymbolSelectionDialog(FrontEnd::Lib::Engine &, QWidget *parent);

 public:
  boost::optional<QString> RequestSymbol();

 private:
  Ui::SymbolSelectionDialog m_ui;
};
}  // namespace MarketMaker
}  // namespace Strategies
}  // namespace trdk
