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

using namespace trdk;
using namespace trdk::Lib;

class Service::Implementation : private boost::noncopyable {

private:

	class FindSubscribedModule
			: public boost::static_visitor<bool>,
			private boost::noncopyable {
	public:
		typedef std::list<Subscriber> Path;
	public:
		explicit FindSubscribedModule(const Subscriber &subscriberToFind)
				: m_subscriberToFind(subscriberToFind),
				m_path(*new Path),
				m_isMyPath(true) {
			//...//
		}
		~FindSubscribedModule() {
			if (m_isMyPath) {
				delete &m_path;
			}
		}
	private:
		explicit FindSubscribedModule(
					const Subscriber &subscriberToFind,
					Path &path)
				: m_subscriberToFind(subscriberToFind),
				m_path(path),
				m_isMyPath(false) {
			//...//
		}
	public:
		const Path & GetPath() const {
			return m_path;
		}
	public:
		bool operator ()(const Strategy &) const {
			return false;
		}
		bool operator ()(Service &service) const {
			return Find(service, m_subscriberToFind, m_path);
		}
		bool operator ()(const Observer &) const {
			return false;
		}
	private:
		static bool Find(
					Service &service,
					const Subscriber &subscriberToFind,
					Path &path) {
			path.push_back(ModuleRef(service));
			foreach (const auto &subscriber, service.GetSubscribers()) {
				if (subscriber == subscriberToFind) {
					path.push_back(subscriber);
					return true;
				} else if (
						boost::apply_visitor(
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
	bool m_hasNewData;

	ModuleSecurityList m_securities;
	SubscriberList m_subscribers;

	explicit Implementation(Service &service)
			: m_service(service) {
		//...//
	}

	void CheckRecursiveSubscription(const Subscriber &subscriber) const {
		const FindSubscribedModule find(ModuleRef(m_service));
 		if (!boost::apply_visitor(find, subscriber)) {
			return;
		}
 		Assert(!find.GetPath().empty());
		std::list<std::string> path;
		foreach (const auto &i, find.GetPath()) {
			std::ostringstream oss;
			oss
				<< '"'
				<< boost::apply_visitor(Visitors::GetModule(), i)
				<< '"';
			path.push_back(oss.str());
		}
		path.reverse();
		m_service.GetLog().Error(
			"Recursive service reference detected:"
				" trying to make subscription \"%1%\" -> \"%2%\","
				" but already exists subscription %3%.",
			boost::make_tuple(
				boost::cref(boost::apply_visitor(Visitors::GetModule(), subscriber)),
				boost::cref(m_service),
				boost::cref(boost::join(path, " -> "))));
		throw Exception("Recursive service reference detected");
	}

	template<typename Module>
	void RegisterSubscriber(Module &module) {
		const Subscriber subscriber(ModuleRef(module));
		CheckRecursiveSubscription(subscriber);
		m_subscribers.push_back(subscriber);
		module.OnServiceStart(m_service);
	}

};

Service::Service(
			Context &context,
			const std::string &name,
			const std::string &tag)
		: Module(context, "Service", name, tag) {
	m_pimpl = new Implementation(*this);
}

Service::~Service() {
	delete m_pimpl;
}

pt::ptime Service::OnSecurityStart(const Security &) {
	return pt::not_a_date_time;
}

bool Service::RaiseLevel1UpdateEvent(const Security &security) {
	const Lock lock(GetMutex());
	return OnLevel1Update(security);
}

bool Service::RaiseLevel1TickEvent(
			const Security &security,
			const boost::posix_time::ptime &time,
			const Level1TickValue &value) {
	const Lock lock(GetMutex());
	return OnLevel1Tick(security, time, value);
}

bool Service::RaiseNewTradeEvent(
			const Security &security,
			const boost::posix_time::ptime &time,
			ScaledPrice price,
			Qty qty,
			OrderSide side) {
	const Lock lock(GetMutex());
	return OnNewTrade(security, time, price, qty, side);
}

bool Service::RaiseServiceDataUpdateEvent(const Service &service) {
	const Lock lock(GetMutex());
	return OnServiceDataUpdate(service);
}

bool Service::RaiseBrokerPositionUpdateEvent(
			Security &security,
			Qty qty,
			bool isInitial) {
	const Lock lock(GetMutex());
	return OnBrokerPositionUpdate(security, qty, isInitial);
}

bool Service::RaiseNewBarEvent(
			const Security &security,
			const Security::Bar &bar) {
	const Lock lock(GetMutex());
	return OnNewBar(security, bar);
}

bool Service::OnLevel1Update(const Security &security) {
	GetLog().Error(
		"Subscribed to %1% Level 1 Updates, but can't work with it"
			" (hasn't OnLevel1Update method implementation).",
		security);
	throw MethodDoesNotImplementedError(
		"Module subscribed to Level 1 Updates, but can't work with it");
}

bool Service::OnLevel1Tick(
			const Security &security,
			const boost::posix_time::ptime &,
			const Level1TickValue &) {
	GetLog().Error(
		"Subscribed to %1% Level 1 Ticks, but can't work with it"
			" (hasn't OnLevel1Tick method implementation).",
			security);
	throw MethodDoesNotImplementedError(
		"Module subscribed to Level 1 Ticks, but can't work with it");
}

bool Service::OnNewTrade(
					const Security &security,
					const boost::posix_time::ptime &,
					ScaledPrice,
					Qty,
					OrderSide) {
	GetLog().Error(
		"Subscribed to %1% new trades, but can't work with it"
			" (hasn't OnNewTrade method implementation).",
		security);
	throw MethodDoesNotImplementedError(
		"Module subscribed to new trades, but can't work with it");
}

bool Service::OnServiceDataUpdate(const Service &service) {
	GetLog().Error(
		"Subscribed to \"%1%\", but can't work with it"
			" (hasn't OnServiceDataUpdate method implementation).",
		service);
 	throw MethodDoesNotImplementedError(
 		"Module subscribed to service, but can't work with it");
}

bool Service::OnBrokerPositionUpdate(
			Security &security,
			Qty,
			bool /*isInitial*/) {
	GetLog().Error(
		"Subscribed to %1% Broker Positions Updates, but can't work with it"
			" (hasn't OnBrokerPositionUpdate method implementation).",
		security);
	throw MethodDoesNotImplementedError(
		"Module subscribed to Broker Positions Updates"
			", but can't work with it");
}

bool Service::OnNewBar(const Security &security, const Security::Bar &) {
	GetLog().Error(
		"Subscribed to %1% new bars, but can't work with it"
			" (hasn't OnNewBar method implementation).",
		security);
	throw MethodDoesNotImplementedError(
		"Module subscribed to new bars, but can't work with it");
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

const Service::SubscriberList & Service::GetSubscribers() {
	return m_pimpl->m_subscribers;
}

void Service::RegisterSource(Security &security) {
	if (!m_pimpl->m_securities.Insert(security)) {
		return;
	}
	const auto dataStart = OnSecurityStart(security);
	if (dataStart != pt::not_a_date_time) {
		security.SetRequestedDataStartTime(dataStart);
	}
}

const Service::SecurityList & Service::GetSecurities() const {
	return m_pimpl->m_securities;
}
