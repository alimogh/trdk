/*******************************************************************************
 *   Created: 2017/10/25 23:25:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "GeneratedFiles/ui_GeneralSetupDialog.h"

namespace trdk {
namespace FrontEnd {
namespace WilliamCarry {

class GeneralSetupDialog : public QDialog {
  Q_OBJECT

 public:
  explicit GeneralSetupDialog(const std::vector<trdk::Lib::Double> &,
                              QWidget *parent);

 public:
  std::vector<trdk::Lib::Double> GetLots() const;

 private:
  Ui::GeneralSetupDialog m_ui;
};
}
}
}
