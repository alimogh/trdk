/*******************************************************************************
 *   Created: 2017/10/22 17:53:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Advice.hpp"

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {

class Strategy : public trdk::Strategy {
 public:
  struct TradingSettings {
    bool isEnabled;
    Lib::Double minPriceDifferenceRatio;
    Qty maxQty;
  };

  static const boost::uuids::uuid typeId;

  typedef trdk::Strategy Base;

  explicit Strategy(Context &,
                    const std::string &instanceName,
                    const boost::property_tree::ptree &config);
  ~Strategy() override;

  boost::signals2::scoped_connection SubscribeToAdvice(
      const boost::function<void(const Advice &)> &);
  boost::signals2::scoped_connection SubscribeToTradingSignalCheckErrors(
      const boost::function<void(const std::vector<std::string> &)> &);
  boost::signals2::scoped_connection SubscribeToBlocking(
      const boost::function<void(const std::string *reason)> &);

  void SetMinPriceDifferenceRatioToAdvice(const Lib::Double &);
  const Lib::Double &GetMinPriceDifferenceRatioToAdvice() const;

  void SetTradingSettings(TradingSettings &&);
  const TradingSettings &GetTradingSettings() const;

  bool IsLowestSpreadEnabled() const;
  const Lib::Double &GetLowestSpreadRatio() const;

  void ForEachSecurity(const Lib::Symbol &,
                       const boost::function<void(Security &)> &);

 protected:
  void OnSecurityStart(Security &, Security::Request &) override;
  void OnLevel1Update(Security &,
                      const Lib::TimeMeasurement::Milestones &) override;
  void OnPositionUpdate(Position &) override;
  void OnPostionsCloseRequest() override;
  bool OnBlocked(const std::string *) noexcept override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace ArbitrageAdvisor
}  // namespace Strategies
}  // namespace trdk
