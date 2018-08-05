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

#include "Leg.hpp"
#include "ui_LegSetSelectionDialog.h"

namespace trdk {
namespace Strategies {
namespace TriangularArbitrage {

class LegSetSelectionDialog : public QDialog {
  Q_OBJECT
 public:
  explicit LegSetSelectionDialog(FrontEnd::Engine &, QWidget *parent);

 public:
  std::vector<LegsConf> RequestLegSet();

 private:
  Ui::LegSetSelectionDialog m_ui;
  std::vector<LegsConf> m_legSets;
};
}  // namespace TriangularArbitrage
}  // namespace Strategies
}  // namespace trdk
