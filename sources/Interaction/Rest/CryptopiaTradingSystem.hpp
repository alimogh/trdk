/*******************************************************************************
 *   Created: 2017/12/04 23:40:30
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "CryptopiaRequest.hpp"
#include "CryptopiaUtil.hpp"
#include "NonceStorage.hpp"
#include "PullingTask.hpp"
#include "Settings.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

class CryptopiaTradingSystem : public TradingSystem {
 public:
  typedef TradingSystem Base;

 private:
  typedef boost::mutex OrdersRequestsMutex;
  typedef OrdersRequestsMutex::scoped_lock OrdersRequestsReadLock;
  typedef OrdersRequestsMutex::scoped_lock OrdersRequestsWriteLock;

  struct Settings : public Rest::Settings, public NonceStorage::Settings {
    std::string apiKey;
    std::vector<unsigned char> apiSecret;
    std::vector<std::string> defaultSymbols;

    explicit Settings(const Lib::IniSectionRef &, ModuleEventsLog &);
  };

  class OrderTransactionRequest;

  class PrivateRequest : public CryptopiaRequest {
   public:
    typedef CryptopiaRequest Base;

   public:
    explicit PrivateRequest(const std::string &name,
                            NonceStorage &nonces,
                            const Settings &settings,
                            const std::string &params);
    virtual ~PrivateRequest() override = default;

   public:
    virtual Response Send(Poco::Net::HTTPClientSession &,
                          const Context &) override;

   protected:
    virtual void PrepareRequest(const Poco::Net::HTTPClientSession &,
                                const std::string &,
                                Poco::Net::HTTPRequest &) const override;

   private:
    const Settings &m_settings;
    const std::string m_paramsDigest;
    NonceStorage &m_nonces;
    mutable boost::optional<NonceStorage::TakenValue> m_nonce;
  };

  class AccountRequest : public PrivateRequest {
   public:
    typedef PrivateRequest Base;

   public:
    explicit AccountRequest(const std::string &name,
                            NonceStorage &nonces,
                            const Settings &settings,
                            const std::string &params)
        : Base(name, nonces, settings, params) {}
    virtual ~AccountRequest() override = default;

   protected:
    virtual bool IsPriority() const override { return false; }
  };

  class BalancesRequest : public AccountRequest {
   public:
    typedef AccountRequest Base;

   public:
    explicit BalancesRequest(NonceStorage &nonces, const Settings &settings)
        : Base("GetBalance", nonces, settings, "{}") {}
  };

 public:
  explicit CryptopiaTradingSystem(const App &,
                                  const TradingMode &,
                                  Context &,
                                  const std::string &instanceName,
                                  const Lib::IniSectionRef &);
  virtual ~CryptopiaTradingSystem() override = default;

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
  void UpdateBalances();

  bool UpdateOrders();
  OrderId UpdateOrder(const boost::property_tree::ptree &);

  void SubscribeToOrderUpdates(const CryptopiaProductId &);

 private:
  Settings m_settings;
  NonceStorage m_nonces;
  boost::unordered_map<std::string, CryptopiaProduct> m_products;

  BalancesContainer m_balances;
  BalancesRequest m_balancesRequest;

  OrdersRequestsMutex m_openOrdersRequestMutex;
  size_t m_openOrdersRequestsVersion;
  boost::unordered_map<CryptopiaProductId, boost::shared_ptr<Request>>
      m_openOrdersRequests;

  Poco::Net::HTTPSClientSession m_tradingSession;
  Poco::Net::HTTPSClientSession m_pullingSession;

  PullingTask m_pullingTask;

  Timer::Scope m_timerScope;
};
}
}
}