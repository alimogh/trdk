/*******************************************************************************
 *   Created: 2018/01/31 18:43:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Strategies {
namespace MarketMaker {

class TrendRepeatingStrategy : public trdk::Strategy {
 public:
  typedef Strategy Base;

 public:
  explicit TrendRepeatingStrategy(Context &,
                                  const std::string &instanceName,
                                  const Lib::IniSectionRef &);
  virtual ~TrendRepeatingStrategy() override;

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

}  // namespace MarketMaker
}  // namespace Strategies
}  // namespace trdk