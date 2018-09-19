//
//    Created: 2018/09/18 11:06
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "WalletsConfig.hpp"

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API WalletDepositDialog : public QDialog {
  Q_OBJECT
 public:
  typedef QDialog Base;

  explicit WalletDepositDialog(Engine &,
                               QString symbol,
                               const TradingSystem &,
                               const WalletsConfig &,
                               QWidget *parent);
  WalletDepositDialog(WalletDepositDialog &&) = delete;
  WalletDepositDialog(const WalletDepositDialog &) = delete;
  WalletDepositDialog &operator=(WalletDepositDialog &&) = delete;
  WalletDepositDialog &operator=(const WalletDepositDialog &) = delete;
  ~WalletDepositDialog() override;

  const QString &GetSymbol() const;
  QString GetTargetAddress() const;
  TradingSystem &GetSource() const;
  Volume GetVolume() const;

  void done(int) override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace FrontEnd
}  // namespace trdk
