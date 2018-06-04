//
//    Created: 2018/06/05 12:53 AM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "SymbolDialog.hpp"
#include "GeneratedFiles/ui_SymbolDialog.h"

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;

class SymbolDialog::Implementation {
 public:
  Ui::SymbolDialog m_ui;
};

SymbolDialog::SymbolDialog(QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>()) {
  m_pimpl->m_ui.setupUi(this);
  for (auto i = 0; i < numberOfCurrencies; ++i) {
    m_pimpl->m_ui.currency->addItem(
        QString::fromStdString(ConvertToIso(static_cast<Currency>(i))));
  }
}
SymbolDialog::~SymbolDialog() = default;

QString SymbolDialog::GetSymbol() const {
  return m_pimpl->m_ui.symbol->text() + '_' +
         m_pimpl->m_ui.currency->currentText();
}
