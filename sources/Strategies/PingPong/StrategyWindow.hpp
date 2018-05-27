/*******************************************************************************
/*******************************************************************************
 *   Created: 2018/01/31 21:34:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "ui_StrategyWindow.h"

namespace trdk {
namespace Strategies {
namespace PingPong {

class StrategyWindow : public QMainWindow {
  Q_OBJECT

 public:
  typedef QMainWindow Base;

  explicit StrategyWindow(FrontEnd::Engine &,
                          const QString &symbol,
                          QWidget *parent);
  explicit StrategyWindow(FrontEnd::Engine &,
                          const QUuid &strategyId,
                          const QString &name,
                          const boost::property_tree::ptree &config,
                          QWidget *parent);
  ~StrategyWindow() override;

 protected:
  void closeEvent(QCloseEvent *) override;

 private slots:
  void OnBlocked(const QString &reason);
  void OnStrategyEvent(const QString &);

 signals:
  void Blocked(const QString &reason);
  void StrategyEvent(const QString &);

 private:
  void Init();

  bool LoadExchanges();
  void ConnectSignals();

  Strategy &GenerateNewStrategyInstance();
  Strategy &CreateStrategyInstance(const boost::uuids::uuid &strategyId,
                                   const std::string &name,
                                   const boost::property_tree::ptree &config);
  Strategy &RestoreStrategyInstance(const QUuid &strategyId,
                                    const std::string &name,
                                    const boost::property_tree::ptree &config);

  void Disable();

  boost::property_tree::ptree CreateConfig(
      const boost::uuids::uuid &strategyId,
      bool isLongOpeningEnabled,
      bool isShortOpeningEnabled,
      bool isActivePositionsControlEnabled,
      const Qty &positionSize,
      bool isMaOpeningSignalConfirmationEnabled,
      bool isMaClosingSignalConfirmationEnabled,
      size_t fastMaSize,
      size_t slowMaSize,
      bool isRsiOpeningSignalConfirmationEnabled,
      bool isRsiClosingSignalConfirmationEnabled,
      size_t numberOfRsiPeriods,
      const Lib::Double &rsiOverboughtLevel,
      const Lib::Double &rsiOversoldLevel,
      const Volume &profitShareToActivateTakeProfit,
      const Volume &takeProfitTrailingShareToClose,
      const Lib::Double &maxLossShare,
      const boost::posix_time::time_duration &frameSize) const;
  boost::property_tree::ptree DumpConfig() const;

  void StoreConfig(bool isActive);

  FrontEnd::Engine &m_engine;
  const std::string m_symbol;
  Ui::StrategyWindow m_ui;
  bool m_hasExchanges;

  boost::signals2::scoped_connection m_blockConnection;
  boost::signals2::scoped_connection m_eventsConnection;

  Strategy &m_strategy;
};
}  // namespace PingPong
}  // namespace Strategies
}  // namespace trdk