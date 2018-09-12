//
//    Created: 2018/09/12 9:06 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "WalletSettingsDialog.hpp"
#include "Engine.hpp"
#include "WalletsRechargingConfig.hpp"
#include "ui_WalletSettingsDialog.h"

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;

class WalletSettingsDialog::Implementation {
 public:
  WalletSettingsDialog &m_self;
  QString m_symbol;
  const TradingSystem &m_tradingSystem;
  FrontEnd::Engine &m_engine;
  Ui::WalletSettingsDialog m_ui;

  bool Validate() const {
    if (m_ui.rechargingSettings->isChecked()) {
      if (m_ui.address->text().isEmpty()) {
        QMessageBox::warning(&m_self, tr("Wallet address is not set"),
                             tr("Please provide wallet address."),
                             QMessageBox::Ok);
        m_ui.address->setFocus();
        return false;
      }
      if (m_ui.depositSource->currentIndex() < 0) {
        QMessageBox::warning(&m_self, tr("Deposit source is not set"),
                             tr("Please provide the deposit source."),
                             QMessageBox::Ok);
        m_ui.depositSource->setFocus();
        return false;
      }
      if (Double(m_ui.minDepositVolume->value()) <= 0) {
        QMessageBox::warning(&m_self, tr("Minimal deposit volume is not set"),
                             tr("Please provide a minimum deposit volume to "
                                "start recharge wallet."),
                             QMessageBox::Ok);
        m_ui.minDepositVolume->setFocus();
        return false;
      }
      if (Double(m_ui.minDepositTransactionVolume->value()) <= 0) {
        QMessageBox::warning(
            &m_self, tr("Minimal deposit transaction volume is not set"),
            tr("Please provide a minimum deposit recharging transaction "
               "volume."),
            QMessageBox::Ok);
        m_ui.minDepositTransactionVolume->setFocus();
        return false;
      }
    }
    return true;
  }

  void Store() const {
    auto &config = m_engine.GetWalletsRechargingConfig();
    {
      auto &sources = config.GetSources();
      const auto &it = sources.find(m_symbol);
      if (m_ui.sourceSettings->isChecked()) {
        if (it == sources.constEnd()) {
          sources.insert(m_symbol, {&m_tradingSystem});
        } else {
          it.value().insert(&m_tradingSystem);
        }
      } else if (it != sources.constEnd()) {
        it.value().erase(&m_tradingSystem);
      }
    }
    config.Save();
  }
};

WalletSettingsDialog::WalletSettingsDialog(QString symbol,
                                           const TradingSystem &tradingSystem,
                                           FrontEnd::Engine &engine,
                                           QWidget *parent)
    : Base(parent),
      m_pimpl(boost::make_unique<Implementation>(
          Implementation{*this, std::move(symbol), tradingSystem, engine})) {
  m_pimpl->m_ui.setupUi(this);

  setWindowTitle(
      QString(tr("%1 on %2"))
          .arg(m_pimpl->m_symbol, m_pimpl->m_tradingSystem.GetTitle().c_str()));

  m_pimpl->m_ui.sourceMinDepositSymbol->setText(m_pimpl->m_symbol);
  m_pimpl->m_ui.minDepositVolumeSymbol->setText(m_pimpl->m_symbol);
  m_pimpl->m_ui.minDepositTransactionVolumeSymbol->setText(m_pimpl->m_symbol);

  {
    const auto &config = engine.GetWalletsRechargingConfig();
    const auto &it = config.GetSources().constFind(m_pimpl->m_symbol);
    if (it != config.GetSources().constEnd()) {
      for (const auto &remoteTradingSystem : it.value()) {
        if (remoteTradingSystem == &m_pimpl->m_tradingSystem) {
          m_pimpl->m_ui.sourceSettings->setChecked(true);
        } else {
          m_pimpl->m_ui.depositSource->addItem(
              QString::fromStdString(remoteTradingSystem->GetTitle()));
        }
      }
    }
  }
}

WalletSettingsDialog::~WalletSettingsDialog() = default;

void WalletSettingsDialog::done(const int result) {
  if (Accepted == result) {
    if (!m_pimpl->Validate()) {
      return;
    }
    m_pimpl->Store();
  }
  QDialog::done(result);
}