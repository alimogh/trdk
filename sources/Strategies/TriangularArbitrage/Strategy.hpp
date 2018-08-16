/*******************************************************************************
 *   Created: 2018/03/06 10:01:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Leg.hpp"

namespace trdk {
namespace Strategies {
namespace TriangularArbitrage {

class Strategy : public trdk::Strategy {
 public:
  typedef trdk::Strategy Base;

  static const boost::uuids::uuid typeId;

  explicit Strategy(Context &,
                    const std::string &instanceName,
                    const boost::property_tree::ptree &);
  ~Strategy() override;

  bool IsTradingEnabled() const;
  void EnableTrading(bool);

  const Volume &GetMinVolume() const;
  void SetMinVolume(const Volume &);
  const Volume &GetMaxVolume() const;
  void SetMaxVolume(const Volume &);

  const Lib::Double &GetMinProfitRatio() const;
  void SetMinProfitRatio(const Lib::Double &);

  const boost::unordered_set<size_t> &GetStartExchanges() const;
  void SetStartExchanges(boost::unordered_set<size_t> &&);
  const boost::unordered_set<size_t> &GetMiddleExchanges() const;
  void SetMiddleExchanges(boost::unordered_set<size_t> &&);
  const boost::unordered_set<size_t> &GetFinishExchanges() const;
  void SetFinishExchanges(boost::unordered_set<size_t> &&);

  void Stop() noexcept;

  boost::signals2::scoped_connection SubscribeToOpportunity(
      const boost::function<void(const std::vector<Opportunity> &)> &);
  boost::signals2::scoped_connection SubscribeToBlocking(
      const boost::function<void(const std::string *reason)> &);

  const boost::array<std::unique_ptr<LegPolicy>, numberOfLegs> &GetLegs() const;
  Leg GetLeg(const Security &) const;

 protected:
  void OnSecurityStart(Security &, Security::Request &) override;
  void OnLevel1Update(Security &,
                      const Lib::TimeMeasurement::Milestones &) override;
  void OnPositionUpdate(Position &) override;
  void OnPostionsCloseRequest() override;
  bool OnBlocked(const std::string * = nullptr) noexcept override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace TriangularArbitrage
}  // namespace Strategies
}  // namespace trdk