/*******************************************************************************
 *   Created: 2017/09/10 00:41:14
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Core/TradingSystem.hpp"
#include "ShellApi.h"
#include "ShellFwd.hpp"

namespace trdk {
namespace FrontEnd {
namespace Shell {

class TRDK_FRONTEND_SHELL_LIB_API Engine : public QObject {
  Q_OBJECT

 public:
  explicit Engine(const boost::filesystem::path &configFilePath,
                  QWidget *parent);
  ~Engine();

 public:
  const boost::filesystem::path &GetConfigFilePath() const;

  bool IsStarted() const;

  Context &GetContext();
  const Shell::DropCopy &GetDropCopy() const;

  const TradingSystem::OrderStatusUpdateSlot &GetOrderTradingSystemSlot();

  RiskControlScope &GetRiskControl(const TradingMode &);

 signals:
  void StateChanged(bool isStarted);
  void Message(const QString &, bool isWarning);
  void LogRecord(const QString &);
  void Order(unsigned int id,
             QString tradingSystemOrderId,
             int,
             double remainingQty);
  void Trade(unsigned int orderId,
             QString tradingSystemOrderId,
             int,
             double remainingQty,
             QString tradeId,
             double tradeQty,
             double tradePrice);

 public:
  void Start();
  void Stop();

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
}