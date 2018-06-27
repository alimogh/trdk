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
#include "PollingTask.hpp"
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

  typedef boost::mutex CancelOrderMutex;
  typedef OrdersRequestsMutex::scoped_lock CancelOrderLock;

  struct Settings : Rest::Settings {
    std::string apiKey;
    std::vector<unsigned char> apiSecret;

    explicit Settings(const boost::property_tree::ptree &, ModuleEventsLog &);
  };

  class OrderTransactionRequest;

  class PrivateRequest : public CryptopiaRequest {
   public:
    typedef CryptopiaRequest Base;

    explicit PrivateRequest(const std::string &name,
                            NonceStorage &nonces,
                            const Settings &settings,
                            const std::string &params,
                            bool isPriority,
                            const Context &,
                            ModuleEventsLog &,
                            ModuleTradingLog * = nullptr);
    ~PrivateRequest() override = default;

    Response Send(std::unique_ptr<Poco::Net::HTTPSClientSession> &) override;

   protected:
    void PrepareRequest(const Poco::Net::HTTPClientSession &,
                        const std::string &,
                        Poco::Net::HTTPRequest &) const override;
    bool IsPriority() const override { return m_isPriority; }

   private:
    const Settings &m_settings;
    const bool m_isPriority;
    const std::string m_paramsDigest;
    NonceStorage &m_nonces;
    mutable boost::optional<NonceStorage::TakenValue> m_nonce;
  };

  class BalancesRequest : public PrivateRequest {
   public:
    explicit BalancesRequest(NonceStorage &nonces,
                             const Settings &settings,
                             const Context &context,
                             ModuleEventsLog &log)
        : PrivateRequest(
              "GetBalance", nonces, settings, "{}", false, context, log) {}
  };

  class OpenOrdersRequest : public PrivateRequest {
   public:
    explicit OpenOrdersRequest(const CryptopiaProductId &product,
                               NonceStorage &nonces,
                               const Settings &settings,
                               bool isPriority,
                               const Context &context,
                               ModuleEventsLog &log,
                               ModuleTradingLog &tradingLog)
        : PrivateRequest(
              "GetOpenOrders",
              nonces,
              settings,
              (boost::format("{\"TradePairId\": %1%, \"Count\": 1000}") %
               product)
                  .str(),
              isPriority,
              context,
              log,
              &tradingLog) {}
  };
  class TradesRequest : public PrivateRequest {
   public:
    explicit TradesRequest(const CryptopiaProductId &product,
                           size_t count,
                           NonceStorage &nonces,
                           const Settings &settings,
                           bool isPriority,
                           const Context &context,
                           ModuleEventsLog &log,
                           ModuleTradingLog &tradingLog)
        : PrivateRequest("GetTradeHistory",
                         nonces,
                         settings,
                         (boost::format("{\"TradePairId\":%1%,\"Count\":%2%}") %
                          product % count)
                             .str(),
                         isPriority,
                         context,
                         log,
                         &tradingLog) {}
  };

 public:
  explicit CryptopiaTradingSystem(const App &,
                                  const TradingMode &,
                                  Context &,
                                  std::string instanceName,
                                  std::string title,
                                  const boost::property_tree::ptree &);
  ~CryptopiaTradingSystem() override = default;

  bool IsConnected() const override { return !m_products.empty(); }

  Balances &GetBalancesStorage() override { return m_balances; }

  Volume CalcCommission(const Qty &,
                        const Price &,
                        const OrderSide &,
                        const trdk::Security &) const override;

  boost::optional<OrderCheckError> CheckOrder(const trdk::Security &,
                                              const Lib::Currency &,
                                              const Qty &,
                                              const boost::optional<Price> &,
                                              const OrderSide &) const override;

  bool CheckSymbol(const std::string &) const override;

 protected:
  void CreateConnection() override;

  std::unique_ptr<OrderTransactionContext> SendOrderTransaction(
      trdk::Security &,
      const Lib::Currency &,
      const Qty &,
      const boost::optional<Price> &,
      const OrderParams &,
      const OrderSide &,
      const TimeInForce &) override;

  void SendCancelOrderTransaction(const OrderTransactionContext &) override;

  void OnTransactionSent(const OrderTransactionContext &) override;

 private:
  void UpdateBalances();
  bool UpdateOrders();

  void SubscribeToOrderUpdates(const CryptopiaProductList::const_iterator &);

  boost::optional<OrderId> FindNewOrderId(
      const CryptopiaProductId &,
      const boost::posix_time::ptime &startTime,
      const std::string &side,
      const Qty &,
      const Price &) const;

  boost::posix_time::ptime ParseTimeStamp(
      const boost::property_tree::ptree &) const;

  void RegisterLastOrder(const boost::posix_time::ptime &, const OrderId &);
  bool IsIdRegisterInLastOrders(const OrderId &) const;

  Settings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;
  mutable NonceStorage m_nonces;
  CryptopiaProductList m_products;

  TradingLib::BalancesContainer m_balances;
  BalancesRequest m_balancesRequest;

  OrdersRequestsMutex m_openOrdersRequestMutex;
  size_t m_openOrdersRequestsVersion;
  boost::unordered_map<CryptopiaProductList::const_iterator,
                       boost::shared_ptr<Request>>
      m_openOrdersRequests;

  CancelOrderMutex m_cancelOrderMutex;
  boost::unordered_set<OrderId> m_cancelingOrders;

  std::deque<std::pair<boost::posix_time::ptime, OrderId>> m_lastOrders;

  mutable std::unique_ptr<Poco::Net::HTTPSClientSession> m_tradingSession;
  mutable std::unique_ptr<Poco::Net::HTTPSClientSession> m_pollingSession;

  PollingTask m_pollingTask;

  Timer::Scope m_timerScope;
};

}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk