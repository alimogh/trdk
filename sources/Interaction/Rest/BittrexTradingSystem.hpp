/*******************************************************************************
 *   Created: 2017/11/18 13:15:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "PullingTask.hpp"
#include "Settings.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

class BittrexTradingSystem : public TradingSystem {
 public:
  typedef TradingSystem Base;

 private:
  struct Settings : public Rest::Settings {
    typedef Rest::Settings Base;

    std::string apiKey;
    std::string apiSecret;

    explicit Settings(const Lib::IniSectionRef &, ModuleEventsLog &);
  };

  class PrivateRequest;
  class OrderTransactionRequest;
  class NewOrderRequest;
  class SellOrderRequest;
  class BuyOrderRequest;
  class OrderCancelRequest;
  class AccountRequest;
  class OrderStateRequest;
  class OrderHistoryRequest;
  class BalanceRequest;

 public:
  explicit BittrexTradingSystem(const TradingMode &,
                                size_t index,
                                Context &,
                                const std::string &instanceName,
                                const Lib::IniSectionRef &);
  virtual ~BittrexTradingSystem() override = default;

 public:
  virtual bool IsConnected() const override { return m_isConnected; }

 protected:
  virtual void CreateConnection(const trdk::Lib::IniSectionRef &) override;

  virtual std::unique_ptr<trdk::OrderTransactionContext> SendOrderTransaction(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const boost::optional<trdk::Price> &,
      const trdk::OrderParams &,
      const trdk::OrderSide &,
      const trdk::TimeInForce &) override;

  virtual void SendCancelOrderTransaction(const trdk::OrderId &) override;

 private:
  void UpdateOrders();
  void UpdateOrder(const boost::property_tree::ptree &);

 private:
  Settings m_settings;

  bool m_isConnected;
  Poco::Net::HTTPSClientSession m_tradingSession;
  Poco::Net::HTTPSClientSession m_ordersSession;
  PullingTask m_pullingTask;
};
}
}
}
