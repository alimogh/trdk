//
//    Created: 2018/09/18 11:06
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "WalletDepositDialog.hpp"
#include "Engine.hpp"
#include "ui_WalletDepositDialog.h"

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;

// ReSharper disable CppImplicitDefaultConstructorNotAvailable
class WalletDepositDialog::Implementation {
  // ReSharper restore CppImplicitDefaultConstructorNotAvailable
 public:
  FrontEnd::Engine &m_engine;
  const QString m_symbol;
  Ui::WalletDepositDialog m_ui;
};

WalletDepositDialog::WalletDepositDialog(
    FrontEnd::Engine &engine,
    QString symbol,
    const TradingSystem &targetTradingSystem,
    const WalletsConfig &config,
    QWidget *parent)
    : Base(parent),
      m_pimpl(boost::make_unique<Implementation>(
          Implementation{engine, std::move(symbol), {}})) {
  m_pimpl->m_ui.setupUi(this);

  setWindowTitle(
      QString(tr("Deposit %1 on %2"))
          .arg(m_pimpl->m_symbol, targetTradingSystem.GetTitle().c_str()));

  m_pimpl->m_ui.target->setText(
      config.GetAddress(m_pimpl->m_symbol, targetTradingSystem));

  boost::unordered_map<std::string, bool> symbolsCheckCache;
  for (size_t i = 0; i < engine.GetContext().GetNumberOfTradingSystems(); ++i) {
    const auto &tradingSystem =
        engine.GetContext().GetTradingSystem(i, TRADING_MODE_LIVE);
    if (targetTradingSystem.GetIndex() == tradingSystem.GetIndex() ||
        !tradingSystem.GetAccount().IsWithdrawsFunds()) {
      continue;
    }
    m_pimpl->m_ui.soruce->addItem(
        QString::fromStdString(tradingSystem.GetTitle()),
        tradingSystem.GetIndex());
  }
}
WalletDepositDialog::~WalletDepositDialog() = default;

QString WalletDepositDialog::GetTargetAddress() const {
  return m_pimpl->m_ui.target->text();
}

const QString &WalletDepositDialog::GetSymbol() const {
  return m_pimpl->m_symbol;
}

TradingSystem &WalletDepositDialog::GetSource() const {
  if (m_pimpl->m_ui.soruce->currentIndex() < 0) {
    throw Exception("Deposit source is not set");
  }
  return m_pimpl->m_engine.GetContext().GetTradingSystem(
      m_pimpl->m_ui.soruce->currentData().toULongLong(), TRADING_MODE_LIVE);
}

Volume WalletDepositDialog::GetVolume() const {
  return m_pimpl->m_ui.volume->value();
}

void WalletDepositDialog::done(const int result) {
  if (result == Accepted) {
    if (m_pimpl->m_ui.soruce->currentIndex() < 0) {
      QMessageBox::warning(this, tr("Source is not set"),
                           tr("Please provide the deposit source."),
                           QMessageBox::Ok);
      m_pimpl->m_ui.soruce->setFocus();
      return;
    }
  }
  QDialog::done(result);
}