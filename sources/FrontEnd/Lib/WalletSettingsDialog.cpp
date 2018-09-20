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
#include "WalletsConfig.hpp"
#include "ui_WalletSettingsDialog.h"

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;

// ReSharper disable CppImplicitDefaultConstructorNotAvailable
class WalletSettingsDialog::Implementation {
  // ReSharper restore CppImplicitDefaultConstructorNotAvailable
 public:
  WalletSettingsDialog &m_self;
  QString m_symbol;
  const TradingSystem &m_tradingSystem;
  WalletsConfig m_config;
  const bool m_isAddressRequired;
  Ui::WalletSettingsDialog m_ui;

  bool Validate() const {
    if (m_isAddressRequired && m_ui.address->text().isEmpty()) {
      QMessageBox::warning(&m_self, tr("Address is not set"),
                           tr("Please provide wallet address."),
                           QMessageBox::Ok);
      m_ui.address->setFocus();
      return false;
    }

    if (m_ui.rechargingSettings->isChecked()) {
      if (m_ui.rechargingSettings->isChecked() &&
          m_ui.address->text().isEmpty()) {
        QMessageBox::warning(
            &m_self, tr("Address is not set"),
            tr("Please provide wallet address or disable wallet recharging."),
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

  void Store() {
    if (!m_ui.address->text().isEmpty()) {
      m_config.SetAddress(m_symbol, m_tradingSystem, m_ui.address->text());
    } else {
      m_config.RemoveAddress(m_symbol, m_tradingSystem);
    }

    if (m_ui.sourceSettings->isChecked()) {
      m_config.SetSource(m_symbol, m_tradingSystem,
                         {m_ui.sourceMinDeposit->value()});
    } else {
      m_config.RemoveSource(m_symbol, m_tradingSystem);
    }

    if (m_ui.rechargingSettings->isChecked()) {
      m_config.SetRecharging(
          m_symbol, m_tradingSystem,
          {&m_tradingSystem.GetContext().GetTradingSystem(
               m_ui.depositSource->currentData().toULongLong(),
               TRADING_MODE_LIVE),
           m_ui.minDepositVolume->value(),
           m_ui.minDepositTransactionVolume->value()});
    } else {
      m_config.RemoveRecharging(m_symbol, m_tradingSystem);
    }
  }
};

WalletSettingsDialog::WalletSettingsDialog(QString symbol,
                                           const TradingSystem &tradingSystem,
                                           WalletsConfig config,
                                           const bool isAddressRequired,
                                           QWidget *parent)
    : Base(parent),
      m_pimpl(
          boost::make_unique<Implementation>(Implementation{*this,
                                                            std::move(symbol),
                                                            tradingSystem,
                                                            std::move(config),
                                                            isAddressRequired,
                                                            {}})) {
  m_pimpl->m_ui.setupUi(this);

  setWindowTitle(
      QString(tr("%1 on %2"))
          .arg(m_pimpl->m_symbol, m_pimpl->m_tradingSystem.GetTitle().c_str()));

  m_pimpl->m_ui.sourceMinDepositSymbol->setText(m_pimpl->m_symbol);
  m_pimpl->m_ui.minDepositVolumeSymbol->setText(m_pimpl->m_symbol);
  m_pimpl->m_ui.minDepositTransactionVolumeSymbol->setText(m_pimpl->m_symbol);

  {
    const auto &it = m_pimpl->m_config.Get().constFind(m_pimpl->m_symbol);
    const TradingSystem *rechargingSource = nullptr;
    if (it != m_pimpl->m_config.Get().constEnd()) {
      for (const auto &remoteTradingSystem : it.value()) {
        if (remoteTradingSystem.first == &m_pimpl->m_tradingSystem) {
          if (remoteTradingSystem.second.source) {
            m_pimpl->m_ui.sourceSettings->setChecked(true);
            m_pimpl->m_ui.sourceMinDeposit->setValue(
                remoteTradingSystem.second.source->minDeposit);
          }
          if (remoteTradingSystem.second.wallet.address) {
            m_pimpl->m_ui.address->setText(
                *remoteTradingSystem.second.wallet.address);
          }
          if (remoteTradingSystem.second.wallet.recharging) {
            m_pimpl->m_ui.rechargingSettings->setChecked(true);
            m_pimpl->m_ui.minDepositTransactionVolume->setValue(
                remoteTradingSystem.second.wallet.recharging
                    ->minRechargingTransactionVolume);
            m_pimpl->m_ui.minDepositVolume->setValue(
                remoteTradingSystem.second.wallet.recharging
                    ->minDepositToRecharge);
            rechargingSource =
                remoteTradingSystem.second.wallet.recharging->source;
          }
        } else {
          m_pimpl->m_ui.depositSource->addItem(
              QString::fromStdString(remoteTradingSystem.first->GetTitle()),
              remoteTradingSystem.first->GetIndex());
        }
      }
    }
    if (rechargingSource) {
      for (auto i = 0; i < m_pimpl->m_ui.depositSource->count(); ++i) {
        if (m_pimpl->m_ui.depositSource->itemData(i).toULongLong() ==
            rechargingSource->GetIndex()) {
          m_pimpl->m_ui.depositSource->setCurrentIndex(i);
          break;
        }
      }
    }
  }

  Verify(connect(m_pimpl->m_ui.sourceSettings, &QGroupBox::toggled, this,
                 [this](const bool isChecked) {
                   if (isChecked) {
                     if (!m_pimpl->m_tradingSystem.AreWithdrawalSupported()) {
                       QMessageBox::warning(
                           this, tr("Withdrawals are not supported"),
                           tr("Wallet does not support funds withdraw, "
                              "so it cannot be used as a source."),
                           QMessageBox::Ok);
                       m_pimpl->m_ui.sourceSettings->setChecked(false);
                     }
                   }
                 }));
}

WalletSettingsDialog::~WalletSettingsDialog() = default;

void WalletSettingsDialog::done(const int result) {
  if (result == Accepted) {
    if (!m_pimpl->Validate()) {
      return;
    }
    m_pimpl->Store();
  }
  QDialog::done(result);
}

const WalletsConfig &WalletSettingsDialog::GetConfig() const {
  return m_pimpl->m_config;
}