/*******************************************************************************
 *   Created: 2017/10/01 18:32:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once
#include "Prec.hpp"
#include "GeneratedFiles/ui_MultiBrokerWidget.h"

namespace trdk {
namespace FrontEnd {
namespace WilliamCarry {

class MultiBrokerWidget : public QWidget {
  Q_OBJECT

 private:
  struct Locked {
    boost::posix_time::ptime time;
    Price bid;
    Price ask;
  };

 public:
  explicit MultiBrokerWidget(FrontEnd::Shell::Engine &, QWidget *parent);
  virtual ~MultiBrokerWidget() override = default;

 public:
  TradingSystem *GetSelectedTradingSystem();

  void SetCurrentSecurity(Security *);

 protected slots:
  void LockSecurity(bool lock);
  void OnStateChanged(bool isStarted);
  void UpdatePrices(const Security *);

 private:
  void Reload();
  void ReloadSecurityList();
  void SendBuyOrder(size_t strategyIndex);
  void SendSellOrder(size_t strategyIndex);
  void CloseAllPositions();

  void SetCurretPrices(const boost::posix_time::ptime &,
                       const Price &bid,
                       const Price &ask,
                       const Security *);
  void SetLockedPrices(const Locked &, const Security &);
  void ResetLockedPrices(const Security *);
  void SetPrices(const boost::posix_time::ptime &,
                 QLabel &timeControl,
                 const Price &bid,
                 QLabel &bidControl,
                 const Price &ask,
                 QLabel &askControl,
                 const Security *) const;

 private:
  const TradingMode m_mode;
  Ui::MultiBrokerWidget m_ui;
  Shell::Engine &m_engine;

  Security *m_currentSecurity;
  std::vector<Security *> m_securityList;

  boost::unordered_map<Security *, Locked> m_lockedSecurityList;
  bool m_ignoreLockToggling;
};
}
}
}