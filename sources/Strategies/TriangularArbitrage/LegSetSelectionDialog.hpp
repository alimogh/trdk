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

  boost::optional<LegsConf> RequestLegSet();

 private:
  void LoadPairs();
  void ResetLists();
  void ConnectSiganls();
  void UpdateSides(const OrderSide &, const OrderSide &, const OrderSide &);
  void UpdateSymbolsByLeg1();
  void UpdateSymbolsByLeg2();
  void UpdateOkButton();
  void FilterLeg1Symbols();

  const FrontEnd::Engine &m_engine;

  std::vector<QString> m_leg1Pairs;
  QMap<QString, std::vector<QString>> m_leg2Pairs;

  Ui::LegSetSelectionDialog m_ui{};
};
}  // namespace TriangularArbitrage
}  // namespace Strategies
}  // namespace trdk
