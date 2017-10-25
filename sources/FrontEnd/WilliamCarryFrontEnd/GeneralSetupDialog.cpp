/*******************************************************************************
 *   Created: 2017/10/25 23:30:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "GeneralSetupDialog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd;
using namespace trdk::FrontEnd::WilliamCarry;

GeneralSetupDialog::GeneralSetupDialog(const std::vector<Double> &lots,
                                       QWidget *parent)
    : QDialog(parent) {
  m_ui.setupUi(this);
  if (lots.size() >= 0) {
    m_ui.broker1Lots->setValue(lots[0]);
  }
  if (lots.size() >= 1) {
    m_ui.broker2Lots->setValue(lots[1]);
  }
  if (lots.size() >= 2) {
    m_ui.broker3Lots->setValue(lots[2]);
  }
  if (lots.size() >= 3) {
    m_ui.broker4Lots->setValue(lots[3]);
  }
  if (lots.size() >= 4) {
    m_ui.broker5Lots->setValue(lots[4]);
  }
}

std::vector<Double> GeneralSetupDialog::GetLots() const {
  return {m_ui.broker1Lots->value(), m_ui.broker2Lots->value(),
          m_ui.broker3Lots->value(), m_ui.broker4Lots->value(),
          m_ui.broker5Lots->value()};
}
