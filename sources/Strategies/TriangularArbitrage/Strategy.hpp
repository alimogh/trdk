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

 public:
  explicit Strategy(Context &,
                    const std::string &instanceName,
                    const Lib::IniSectionRef &);
  virtual ~Strategy() override;

 public:
  bool IsTradingEnabled() const;
  void EnableTrading(bool);

  const Volume &GetMinVolume() const;
  void SetMinVolume(const Volume &);
  const Volume &GetMaxVolume() const;
  void SetMaxVolume(const Volume &);

  const Lib::Double &GetMinProfitRatio() const;
  void SetMinProfitRatio(const Lib::Double &);

  boost::optional<size_t> GetStartExchange() const;
  void SetStartExchange(const boost::optional<size_t> &);
  boost::optional<size_t> GetMiddleExchange() const;
  void SetMiddleExchange(const boost::optional<size_t> &);
  boost::optional<size_t> GetFinishExchange() const;
  void SetFinishExchange(const boost::optional<size_t> &);

  void Stop() noexcept;

  boost::signals2::scoped_connection SubscribeToOpportunity(
      const boost::function<void(const std::vector<Opportunity> &)> &);
  boost::signals2::scoped_connection SubscribeToTradingSignalCheckErrors(
      const boost::function<void(const std::vector<std::string> &)> &);
  boost::signals2::scoped_connection SubscribeToBlocking(
      const boost::function<void(const std::string *reason)> &);

  const boost::array<std::unique_ptr<LegPolicy>, numberOfLegs> &GetLegs() const;

 protected:
  virtual void OnSecurityStart(trdk::Security &,
                               trdk::Security::Request &) override;
  virtual void OnLevel1Update(
      trdk::Security &, const Lib::TimeMeasurement::Milestones &) override;
  virtual void OnPositionUpdate(trdk::Position &) override;
  void OnPostionsCloseRequest() override;
  virtual bool OnBlocked(const std::string * = nullptr) noexcept override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace TriangularArbitrage
}  // namespace Strategies
}  // namespace trdk