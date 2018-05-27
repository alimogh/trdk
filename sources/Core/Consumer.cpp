/**************************************************************************
 *   Created: 2013/05/14 07:19:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Consumer.hpp"
#include "ModuleSecurityList.hpp"
#include "Security.hpp"

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;
using namespace trdk;
using namespace Lib;

////////////////////////////////////////////////////////////////////////////////

class Consumer::Implementation : private boost::noncopyable {
 public:
  ModuleSecurityList m_securities;
};

////////////////////////////////////////////////////////////////////////////////

Consumer::Consumer(Context &context,
                   const std::string &typeName,
                   const std::string &name,
                   const std::string &instanceName,
                   const ptr::ptree &conf)
    : Module(context, typeName, name, instanceName, conf),
      m_pimpl(new Implementation) {}

Consumer::~Consumer() = default;

void Consumer::OnSecurityStart(Security &, Security::Request &) {}

void Consumer::OnSecurityContractSwitched(const pt::ptime &,
                                          Security &security,
                                          Security::Request &,
                                          bool &) {
  GetLog().Error(
      "Subscribed to %1% contract switch event, but can't work with it"
      " (doesn't have OnSecurityContractSwitched method implementation).",
      security);
  throw MethodIsNotImplementedException(
      "Module subscribed to contract switch event, but can't work with it");
}

void Consumer::OnLevel1Tick(Security &security,
                            const pt::ptime &,
                            const Level1TickValue &,
                            const TimeMeasurement::Milestones &) {
  GetLog().Error(
      "Subscribed to %1% level 1 ticks, but can't work with it"
      " (doesn't have OnLevel1Tick method implementation).",
      security);
  throw MethodIsNotImplementedException(
      "Module subscribed to Level 1 Ticks, but can't work with it");
}

void Consumer::OnNewTrade(Security &security,
                          const pt::ptime &,
                          const Price &,
                          const Qty &) {
  GetLog().Error(
      "Subscribed to %1% new trades, but can't work with it"
      " (doesn't have OnNewTrade method implementation).",
      security);
  throw MethodIsNotImplementedException(
      "Module subscribed to new trades, but can't work with it");
}

void Consumer::OnBrokerPositionUpdate(
    Security &security, bool, const Qty &, const Volume &, bool) {
  GetLog().Error(
      "Subscribed to %1% broker positions updates, but can't work with it"
      " (doesn't have OnBrokerPositionUpdate method implementation).",
      security);
  throw MethodIsNotImplementedException(
      "Module subscribed to Broker Positions Updates, but can't work with it");
}

void Consumer::OnBarUpdate(Security &security, const Bar &) {
  GetLog().Error(
      "Subscribed to %1% bars, but can't work with it (doesn't have OnNewBar "
      "method implementation).",
      security);
  throw MethodIsNotImplementedException(
      "Module subscribed to new bars, but can't work with it");
}

void Consumer::OnSecurityServiceEvent(const pt::ptime &,
                                      Security &,
                                      const Security::ServiceEvent &) {}

void Consumer::RegisterSource(Security &security) {
  if (!m_pimpl->m_securities.Insert(security)) {
    return;
  }
  Security::Request request;
  OnSecurityStart(security, request);
  security.SetRequest(request);
}

Consumer::SecurityList &Consumer::GetSecurities() {
  return m_pimpl->m_securities;
}

const Consumer::SecurityList &Consumer::GetSecurities() const {
  return const_cast<Consumer *>(this)->GetSecurities();
}

////////////////////////////////////////////////////////////////////////////////
