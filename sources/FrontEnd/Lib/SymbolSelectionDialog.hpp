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

#include "Api.h"
#include "Fwd.hpp"

namespace Ui {
class SymbolSelectionDialog;
}

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API SymbolSelectionDialog : public QDialog {
  Q_OBJECT
 public:
  typedef QDialog Base;

 public:
  explicit SymbolSelectionDialog(Engine &, QWidget *parent);
  ~SymbolSelectionDialog();

 public:
  std::vector<QString> RequestSymbols();

 private:
  std::unique_ptr<Ui::SymbolSelectionDialog> m_ui;
};
}  // namespace FrontEnd
}  // namespace trdk
