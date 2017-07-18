/**************************************************************************
 *   Created: 2016/10/30 17:40:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"
#include "TransaqConnector.hpp"
#include "TransaqSecurity.hpp"

namespace trdk {
namespace Interaction {
namespace Transaq {

class MarketDataSource : public trdk::MarketDataSource,
                         protected MarketDataSourceConnector {
 public:
  explicit MarketDataSource(size_t index,
                            trdk::Context &,
                            const std::string &instanceName);
  virtual ~MarketDataSource();

 public:
  using trdk::MarketDataSource::GetContext;
  using trdk::MarketDataSource::GetLog;

 public:
  virtual void Connect(const trdk::Lib::IniSectionRef &) override;

  virtual void SubscribeToSecurities() override;

 protected:
  virtual trdk::Security &CreateNewSecurityObject(
      const trdk::Lib::Symbol &) override;

  virtual void OnReconnect() override;

  virtual void OnNewTick(const boost::posix_time::ptime &,
                         const std::string &board,
                         const std::string &symbol,
                         double price,
                         const Qty &,
                         const Lib::TimeMeasurement::Milestones &) override;
  virtual void OnLevel1Update(
      const std::string &board,
      const std::string &symbol,
      boost::optional<double> &&bidPrice,
      boost::optional<double> &&bidQty,
      boost::optional<double> &&askPrice,
      boost::optional<double> &&askQty,
      const Lib::TimeMeasurement::Milestones &) override;

 private:
  Transaq::Security &GetSecurity(const std::string &board,
                                 const std::string &symbol);

 private:
  boost::unordered_map<std::pair<std::string, std::string>,
                       boost::shared_ptr<Transaq::Security>>
      m_securities;
};
}
}
}
