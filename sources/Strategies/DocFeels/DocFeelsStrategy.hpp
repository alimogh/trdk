/*******************************************************************************
 *   Created: 2017/09/17 20:31:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Core/Strategy.hpp"
#include "Services/BarService.hpp"
#include "Api.h"
#include "DocFeelsPositionController.hpp"

namespace trdk {
namespace Strategies {
namespace DocFeels {

class TRDK_STRATEGY_DOCFEELS_API Strategy : public trdk::Strategy {
 public:
  typedef trdk::Strategy Base;

 public:
  explicit Strategy(Context &,
                    const std::string &typeUuid,
                    const std::string &implementationName,
                    const std::string &instanceName,
                    const Lib::IniSectionRef &,
                    const boost::shared_ptr<TradingLib::Trend> &);
  virtual ~Strategy() override = default;

 protected:
  virtual void OnSecurityStart(trdk::Security &,
                               trdk::Security::Request &) override;
  virtual void OnServiceStart(const trdk::Service &) override;
  virtual void OnServiceDataUpdate(
      const trdk::Service &,
      const trdk::Lib::TimeMeasurement::Milestones &) override;
  virtual void OnPositionUpdate(trdk::Position &) override;
  virtual void OnPostionsCloseRequest() override;

 protected:
  const Security &GetSecurity() const {
    Assert(m_security);
    return *m_security;
  }

  const Services::BarService &GetBarService() const {
    Assert(m_barService);
    Assert(!m_barService->IsEmpty());
    return *m_barService;
  }

  template <typename Trend>
  Trend &GetTypedTrend() {
    return *boost::polymorphic_downcast<Trend *>(&*m_trend);
  }
  template <typename Trend>
  const Trend &GetTypedTrend() const {
    return const_cast<Strategy *>(this)->GetTypedTrend<Trend>();
  }

 protected:
  virtual void CheckSignal(const Lib::TimeMeasurement::Milestones &) = 0;

  void OnSignal(const trdk::Lib::TimeMeasurement::Milestones &);

 private:
  const boost::shared_ptr<TradingLib::Trend> m_trend;
  PositionController m_positionController;
  Security *m_security;
  const Services::BarService *m_barService;
};
}
}
}