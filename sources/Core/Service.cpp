/**************************************************************************
 *   Created: 2012/11/03 18:26:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Service.hpp"
#include "Observer.hpp"
#include "Strategy.hpp"

using namespace Trader;
using namespace Trader::Lib;

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
		mutable Path &m_path;
		const bool m_isMyPath;
	};

public:

	Service &m_service;
	boost::shared_ptr<const Security> m_security;
	bool m_hasNewData;
	Subscribers m_subscribers;

	explicit Implementation(
				Service &service,
				const boost::shared_ptr<const Security> &security)
			: m_service(service),
			m_security(security) {
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
		Assert(
			std::find(m_subscribers.begin(), m_subscribers.end(), subscriber)
			== m_subscribers.end());
		m_subscribers.push_back(subscriber);
	}

};

Service::Service(
			const std::string &name,
			const std::string &tag,
			boost::shared_ptr<const Security> security,
			boost::shared_ptr<const Settings> settings)
		: SecurityAlgo("Service", name, tag, settings) {
	m_pimpl = new Implementation(*this, security);
}

Service::~Service() {
	delete m_pimpl;
}

const Security & Service::GetSecurity() const {
	return *m_pimpl->m_security;
}

bool Service::RaiseLevel1UpdateEvent(const Security &security) {
	const Lock lock(GetMutex());
	return OnLevel1Update(security);
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

bool Service::OnLevel1Update(const Security &) {
	GetLog().Error(
		"Subscribed to Level 1 updates, but can't work with it"
			" (hasn't OnLevel1Update method implementation).");
	throw MethodDoesNotImplementedError(
		"Module subscribed to Level 1 updates, but can't work with it");
}

bool Service::OnNewTrade(
					const Security &,
					const boost::posix_time::ptime &,
					ScaledPrice,
					Qty,
					OrderSide) {
	GetLog().Error(
		"Subscribed to new trades, but can't work with it"
			" (hasn't OnNewTrade method implementation).");
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

void Service::RegisterSubscriber(Strategy &module) {
	m_pimpl->RegisterSubscriber(module);
}

void Service::RegisterSubscriber(Service &module) {
	m_pimpl->RegisterSubscriber(module);
}

void Service::RegisterSubscriber(Observer &module) {
	m_pimpl->RegisterSubscriber(module);
}

const Service::Subscribers & Service::GetSubscribers() {
	return m_pimpl->m_subscribers;
}
