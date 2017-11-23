/*******************************************************************************
 *   Created: 2017/10/12 11:27:54
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
namespace WilliamCarry {

class MultibrokerStrategy : public Strategy {
 public:
  typedef Strategy Base;

 public:
  explicit MultibrokerStrategy(Context &,
                               const std::string &instanceName,
                               const Lib::IniSectionRef &);
  virtual ~MultibrokerStrategy() override;

 public:
  void OpenPosition(std::vector<OperationContext> &&,
                    Security &,
                    bool isLong,
                    const Lib::TimeMeasurement::Milestones &);

  boost::signals2::scoped_connection SubscribeToPositionsUpdates(
      const boost::function<void(bool isLong, bool isActive)> &) const;

 protected:
  virtual void OnLevel1Tick(
      trdk::Security &,
      const boost::posix_time::ptime &,
      const trdk::Level1TickValue &,
      const trdk::Lib::TimeMeasurement::Milestones &) override;
  virtual void OnPositionUpdate(trdk::Position &) override;
  void OnPostionsCloseRequest() override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
}
