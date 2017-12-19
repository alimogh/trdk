/*******************************************************************************
 *   Created: 2017/12/19 21:00:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "LivecoinRequest.hpp"
#include "LivecoinUtil.hpp"
#include "PullingTask.hpp"
#include "Settings.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

class LivecoinTradingSystem : public TradingSystem {
 public:
  typedef TradingSystem Base;

  struct Settings : public Rest::Settings {
    std::string apiKey;
    std::vector<unsigned char> apiSecret;

    explicit Settings(const Lib::IniSectionRef &, ModuleEventsLog &);
  };

 public:
  explicit LivecoinTradingSystem(const App &,
                                 const TradingMode &,
                                 Context &,
                                 const std::string &instanceName,
                                 const Lib::IniSectionRef &);
  virtual ~LivecoinTradingSystem() override = default;

 public:
  virtual bool IsConnected() const override { return !m_products.empty(); }

  virtual Balances &GetBalancesStorage() override { return m_balances; }

  virtual Volume CalcCommission(const Volume &,
                                const trdk::Security &) const override;

  virtual boost::optional<OrderCheckError> CheckOrder(
      const trdk::Security &,
      const Lib::Currency &,
      const Qty &,
      const boost::optional<Price> &,
      const OrderSide &) const override;

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

  virtual void OnTransactionSent(const trdk::OrderId &) override;

 private:
  Settings m_settings;
  boost::unordered_map<std::string, LivecoinProduct> m_products;

  BalancesContainer m_balances;

  Poco::Net::HTTPSClientSession m_tradingSession;
  Poco::Net::HTTPSClientSession m_pullingSession;

  PullingTask m_pullingTask;
};
}
}
}