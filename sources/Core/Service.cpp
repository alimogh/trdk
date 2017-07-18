/**************************************************************************
 *   Created: 2012/11/03 18:26:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Service.hpp"
#include "ModuleSecurityList.hpp"
#include "Observer.hpp"
#include "Strategy.hpp"

namespace pt = boost::posix_time;
namespace uuids = boost::uuids;

using namespace trdk;
using namespace trdk::Lib;

class Service::Implementation : private boost::noncopyable {
 private:
  class FindSubscribedModule : public boost::static_visitor<bool>,
                               private boost::noncopyable {
   public:
    typedef std::list<Subscriber> Path;

   public:
    explicit FindSubscribedModule(const Subscriber &subscriberToFind)
        : m_subscriberToFind(subscriberToFind),
          m_path(*new Path),
          m_isMyPath(true) {}
    ~FindSubscribedModule() {
      if (m_isMyPath) {
        delete &m_path;
      }
    }

   private:
    explicit FindSubscribedModule(const Subscriber &subscriberToFind,
                                  Path &path)
        : m_subscriberToFind(subscriberToFind),
          m_path(path),
          m_isMyPath(false) {}

   public:
    const Path &GetPath() const { return m_path; }

   public:
    bool operator()(const Strategy &) const { return false; }
    bool operator()(Service &service) const {
      return Find(service, m_subscriberToFind, m_path);
    }
    bool operator()(const Observer &) const { return false; }

   private:
    static bool Find(Service &service,
                     const Subscriber &subscriberToFind,
                     Path &path) {
      path.push_back(ModuleRef(service));
      foreach (const auto &subscriber, service.GetSubscribers()) {
        if (subscriber == subscriberToFind) {
          path.push_back(subscriber);
          return true;
        } else if (boost::apply_visitor(
                       FindSubscribedModule(subscriberToFind, path),
                       subscriber)) {
          return true;
        }
      }
      return false;
    }

   private:
    const Subscriber &m_subscriberToFind;
    Path &m_path;
    const bool m_isMyPath;
  };

 public:
  Service &m_service;
  const uuids::uuid m_typeId;
  const std::string m_tag;

  ModuleSecurityList m_securities;
  SubscriberList m_subscribers;

  explicit Implementation(Service &service,
                          const uuids::uuid &typeId,
                          const IniSectionRef &conf)
      : m_service(service), m_typeId(typeId), m_tag(conf.ReadKey("tag", "")) {}

  void CheckRecursiveSubscription(const Subscriber &subscriber) const {
    const FindSubscribedModule find(ModuleRef(m_service));
    if (!boost::apply_visitor(find, subscriber)) {
      return;
    }
    Assert(!find.GetPath().empty());
    std::list<std::string> path;
    foreach (const auto &i, find.GetPath()) {
      std::ostringstream oss;
      oss << '"' << boost::apply_visitor(Visitors::GetModule(), i) << '"';
      path.push_back(oss.str());
    }
    path.reverse();
    m_service.GetLog().Error(
        "Recursive service reference detected:"
        " trying to make subscription \"%1%\" -> \"%2%\","
        " but already exists subscription %3%.",
        boost::apply_visitor(Visitors::GetModule(), subscriber), m_service,
        boost::join(path, " -> "));
    throw Exception("Recursive service reference detected");
  }

  template <typename Module>
  void RegisterSubscriber(Module &module) {
    const Subscriber subscriber(ModuleRef(module));
    CheckRecursiveSubscription(subscriber);
    m_subscribers.push_back(subscriber);
    module.OnServiceStart(m_service);
  }
};

Service::Service(Context &context,
                 const uuids::uuid &typeId,
                 const std::string &implementationName,
                 const std::string &instanceName,
                 const IniSectionRef &conf)
    : Module(context, "Service", implementationName, instanceName, conf),
      m_pimpl(boost::make_unique<Implementation>(*this, typeId, conf)) {}

Service::~Service() {}

const uuids::uuid &Service::GetTypeId() const { return m_pimpl->m_typeId; }

const std::string &Service::GetTag() const { return m_pimpl->m_tag; }

void Service::OnSecurityStart(const Security &, Security::Request &) {}

void Service::OnSecurityContractSwitched(const pt::ptime &,
                                         const Security &security,
                                         Security::Request &) {
  GetLog().Error(
      "Subscribed to %1% contract switch event, but can't work with it"
      " (doesn't have OnSecurityContractSwitched method implementation).",
      security);
  throw MethodDoesNotImplementedError(
      "Module subscribed to contract switch event, but can't work with it");
}

void Service::RaiseSecurityContractSwitchedEvent(const pt::ptime &time,
                                                 const Security &security,
                                                 Security::Request &request) {
  const auto lock = LockForOtherThreads();
  OnSecurityContractSwitched(time, security, request);
}

bool Service::RaiseLevel1UpdateEvent(const Security &security) {
  const auto lock = LockForOtherThreads();
  return OnLevel1Update(security);
}

bool Service::RaiseLevel1TickEvent(const Security &security,
                                   const boost::posix_time::ptime &time,
                                   const Level1TickValue &value) {
  const auto lock = LockForOtherThreads();
  return OnLevel1Tick(security, time, value);
}

bool Service::RaiseNewTradeEvent(const Security &security,
                                 const boost::posix_time::ptime &time,
                                 const ScaledPrice &price,
                                 const Qty &qty) {
  const auto lock = LockForOtherThreads();
  return OnNewTrade(security, time, price, qty);
}

bool Service::RaiseServiceDataUpdateEvent(
    const Service &service,
    const TimeMeasurement::Milestones &timeMeasurement) {
  const auto lock = LockForOtherThreads();
  return OnServiceDataUpdate(service, timeMeasurement);
}

bool Service::RaiseBrokerPositionUpdateEvent(const Security &security,
                                             const Qty &qty,
                                             bool isInitial) {
  const auto lock = LockForOtherThreads();
  return OnBrokerPositionUpdate(security, qty, isInitial);
}

bool Service::RaiseNewBarEvent(const Security &security,
                               const Security::Bar &bar) {
  const auto lock = LockForOtherThreads();
  return OnNewBar(security, bar);
}

bool Service::RaiseBookUpdateTickEvent(
    const Security &security,
    const PriceBook &book,
    const TimeMeasurement::Milestones &timeMeasurement) {
  const auto lock = LockForOtherThreads();
  timeMeasurement.Measure(TimeMeasurement::SM_DISPATCHING_DATA_RAISE);
  return OnBookUpdateTick(security, book, timeMeasurement);
}

bool Service::RaiseSecurityServiceEvent(
    const pt::ptime &time,
    const Security &security,
    const Security::ServiceEvent &securityEvent) {
  const auto lock = LockForOtherThreads();
  return OnSecurityServiceEvent(time, security, securityEvent);
}

bool Service::OnLevel1Update(const Security &security) {
  GetLog().Error(
      "Subscribed to %1% level 1 updates, but can't work with it"
      " (doesn't have OnLevel1Update method implementation).",
      security);
  throw MethodDoesNotImplementedError(
      "Service subscribed to level 1 updates, but can't work with it");
}

bool Service::OnLevel1Tick(const Security &security,
                           const boost::posix_time::ptime &,
                           const Level1TickValue &) {
  GetLog().Error(
      "Subscribed to %1% level 1 ticks, but can't work with it"
      " (doesn't have OnLevel1Tick method implementation).",
      security);
  throw MethodDoesNotImplementedError(
      "Service subscribed to level 1 ticks, but can't work with it");
}

bool Service::OnNewTrade(const Security &security,
                         const boost::posix_time::ptime &,
                         const ScaledPrice &,
                         const Qty &) {
  GetLog().Error(
      "Subscribed to %1% new trades, but can't work with it"
      " (doesn't have OnNewTrade method implementation).",
      security);
  throw MethodDoesNotImplementedError(
      "Service subscribed to new trades, but can't work with it");
}

bool Service::OnServiceDataUpdate(const Service &service,
                                  const TimeMeasurement::Milestones &) {
  GetLog().Error(
      "Subscribed to \"%1%\", but can't work with it"
      " (doesn't have OnServiceDataUpdate method implementation).",
      service);
  throw MethodDoesNotImplementedError(
      "Service subscribed to service, but can't work with it");
}

bool Service::OnBrokerPositionUpdate(const Security &security,
                                     const Qty &,
                                     bool /*isInitial*/) {
  GetLog().Error(
      "Subscribed to %1% broker positions updates, but can't work with it"
      " (doesn't have OnBrokerPositionUpdate method implementation).",
      security);
  throw MethodDoesNotImplementedError(
      "Service subscribed to broker positions updates"
      ", but can't work with it");
}

bool Service::OnNewBar(const Security &security, const Security::Bar &) {
  GetLog().Error(
      "Subscribed to %1% new bars, but can't work with it"
      " (doesn't have OnNewBar method implementation).",
      security);
  throw MethodDoesNotImplementedError(
      "Service subscribed to new bars, but can't work with it");
}

bool Service::OnBookUpdateTick(const Security &security,
                               const PriceBook &,
                               const TimeMeasurement::Milestones &) {
  GetLog().Error(
      "Subscribed to %1% book update ticks, but can't work with it"
      " (doesn't have OnNewBookUpdateTick method implementation).",
      security);
  throw MethodDoesNotImplementedError(
      "Service subscribed to book update ticks, but can't work with it");
}

bool Service::OnSecurityServiceEvent(const pt::ptime &,
                                     const Security &,
                                     const Security::ServiceEvent &) {
  return false;
}

void Service::RegisterSubscriber(Strategy &module) {
  m_pimpl->RegisterSubscriber(module);
}

void Service::RegisterSubscriber(Service &module) {
  m_pimpl->RegisterSubscriber(module);
}

void Service::RegisterSubscriber(Observer &module) {
  m_pimpl->RegisterSubscriber(module);
}

const Service::SubscriberList &Service::GetSubscribers() {
  return m_pimpl->m_subscribers;
}

void Service::RegisterSource(Security &security) {
  if (!m_pimpl->m_securities.Insert(security)) {
    return;
  }
  Security::Request request;
  OnSecurityStart(security, request);
  security.SetRequest(request);
}

const Service::SecurityList &Service::GetSecurities() const {
  return m_pimpl->m_securities;
}
