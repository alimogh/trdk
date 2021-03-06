//
//    Created: 2018/09/12 9:06 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API WalletSettingsDialog : public QDialog {
  Q_OBJECT
 public:
  typedef QDialog Base;

  explicit WalletSettingsDialog(Engine &,
                                QString symbol,
                                const TradingSystem &,
                                WalletsConfig,
                                bool isAddressRequired,
                                QWidget *parent);
  WalletSettingsDialog(WalletSettingsDialog &&) = delete;
  WalletSettingsDialog(const WalletSettingsDialog &) = delete;
  WalletSettingsDialog &operator=(WalletSettingsDialog &&) = delete;
  WalletSettingsDialog &operator=(const WalletSettingsDialog &) = delete;
  ~WalletSettingsDialog() override;

  const WalletsConfig &GetConfig() const;

  void done(int) override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace FrontEnd
}  // namespace trdk
