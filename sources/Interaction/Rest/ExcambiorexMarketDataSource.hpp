/*******************************************************************************
 *   Created: 2018/02/11 17:58:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "ExcambiorexUtil.hpp"
#include "Settings.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

//! ExcambioRex market data source.
class ExcambiorexMarketDataSource : public MarketDataSource {
 public:
  typedef MarketDataSource Base;

 private:
  struct Subscribtion {
    boost::shared_ptr<Security> security;
    const ExcambiorexProduct* product;

    const std::string& GetSymbol() const;
    const std::string& GetProductId() const;
  };

  typedef boost::multi_index_container<
      Subscribtion,
      boost::multi_index::indexed_by<
          boost::multi_index::hashed_unique<
              boost::multi_index::tag<Subscribtion>,
              boost::multi_index::const_mem_fun<Subscribtion,
                                                const std::string&,
                                                &Subscribtion::GetSymbol>>,
          boost::multi_index::hashed_unique<
              boost::multi_index::tag<ById>,
              boost::multi_index::const_mem_fun<Subscribtion,
                                                const std::string&,
                                                &Subscribtion::GetProductId>>>>
      SubscribtionList;

 public:
  explicit ExcambiorexMarketDataSource(const App&,
                                       Context& context,
                                       const std::string& instanceName,
                                       const boost::property_tree::ptree&);
  ~ExcambiorexMarketDataSource() override;

  void Connect() override;

  void SubscribeToSecurities() override;

 protected:
  trdk::Security& CreateNewSecurityObject(const Lib::Symbol&) override;

 private:
  void UpdatePrices(Request&);
  void UpdatePrices(const boost::posix_time::ptime&,
                    const boost::property_tree::ptree&,
                    Security&,
                    const Lib::TimeMeasurement::Milestones&);

  const Settings m_settings;

  std::unique_ptr<Poco::Net::HTTPSClientSession> m_session;

  ExcambiorexProductList m_products;
  boost::mutex m_subscribtionMutex;
  SubscribtionList m_subscribtionList;

  std::unique_ptr<PollingTask> m_pollingTask;
};
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
