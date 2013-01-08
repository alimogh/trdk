/**************************************************************************
 *   Created: 2012/11/22 11:46:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Notifier.hpp"
#include "Core/Strategy.hpp"
#include "Core/Service.hpp"
#include "Core/Observer.hpp"
#include "Core/Position.hpp"

namespace pt = boost::posix_time;

using namespace Trader;
using namespace Trader::Lib;
using namespace Trader::Engine;

//////////////////////////////////////////////////////////////////////////

namespace {

	//////////////////////////////////////////////////////////////////////////

	struct GetModuleVisitor : public boost::static_visitor<Module &> {
		template<typename ModulePtr>
		Module & operator ()(ModulePtr &module) const {
			return *module;
		}
	};

	//////////////////////////////////////////////////////////////////////////

	void NotifyServiceDataUpdateSubscribers(const Service &);

	class NotifyServiceDataUpdateVisitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:
		explicit NotifyServiceDataUpdateVisitor(const Service &service)
				: m_service(service) {
			//...//
		}
	public:		
		void operator ()(const boost::shared_ptr<Strategy> &strategy) const {
			strategy->RaiseServiceDataUpdateEvent(m_service);
		}
		void operator ()(const boost::shared_ptr<Service> &service) const {
			if (!service->RaiseServiceDataUpdateEvent(m_service)) {
				return;
			}
			NotifyServiceDataUpdateSubscribers(*service);
		}
		void operator ()(const boost::shared_ptr<Observer> &observer) const {
			observer->RaiseServiceDataUpdateEvent(m_service);
		}
	private:
		const Service &m_service;
	};

	void NotifyServiceDataUpdateSubscribers(const Service &service) {
		const NotifyServiceDataUpdateVisitor visitor(service);
		foreach (auto subscriber, service.GetSubscribers()) {
			try {
				boost::apply_visitor(visitor, subscriber);
			} catch (const Exception &ex) {
				Log::Error(
					"Error at service subscribers notification:"
						" \"%1%\" (service: \"%2%\", subscriber: \"%3%\").",
					ex,
					service,
					boost::apply_visitor(GetModuleVisitor(), subscriber));
				throw;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////

	class AvailabilityCheckVisitor : public boost::static_visitor<bool> {
	public:
		template<typename Module>
		bool operator ()(const boost::shared_ptr<Module> &) const {
			return true;
		}
		template<>
		bool operator ()(const boost::shared_ptr<Strategy> &strategy) const {
			return !strategy->IsBlocked();
		}
	};

	class BlockVisitor : public boost::static_visitor<void> {
	public:
		template<typename Module>
		void operator ()(const boost::shared_ptr<Module> &) const throw() {
			//...//
		}
		template<>
		void operator ()(
					const boost::shared_ptr<Strategy> &strategy)
				const
				throw() {
			strategy->Block();
		}
	};

	//////////////////////////////////////////////////////////////////////////

}

//////////////////////////////////////////////////////////////////////////

const Module & Notifier::GetObserver() const {
	return boost::apply_visitor(GetModuleVisitor(), m_observer);
}

bool Notifier::IsBlocked() const {
	return !boost::apply_visitor(AvailabilityCheckVisitor(), m_observer);
}

void Notifier::Block() throw() {
	boost::apply_visitor(BlockVisitor(), m_observer);
}

void Notifier::RaiseLevel1UpdateEvent(const Security &security) {

	const class Visitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:		
		explicit Visitor(const Security &security)
				: m_security(security) {
			//...//
		}
	public:
		void operator ()(const boost::shared_ptr<Strategy> &strategy) const {
			AssertEq(
				m_security.GetFullSymbol(),
				strategy->GetSecurity().GetFullSymbol());
			strategy->RaiseLevel1UpdateEvent();
		}
		void operator ()(const boost::shared_ptr<Service> &service) const {
			AssertEq(
				m_security.GetFullSymbol(),
				service->GetSecurity().GetFullSymbol());
			if (service->RaiseLevel1UpdateEvent()) {
				NotifyServiceDataUpdateSubscribers(*service);
			}
		}
		void operator ()(const boost::shared_ptr<Observer> &observer) const {
#			ifdef DEV_VER
			{
				bool isExists = false;
				foreach (const auto &securityPtr, observer->GetNotifyList()) {
					if (&*securityPtr == &m_security) {
						isExists = true;
						break;
					}
				}
				Assert(isExists);
			}
#			endif
			observer->RaiseLevel1UpdateEvent(m_security);
		}
	private:
		const Security &m_security;
	};

	boost::apply_visitor(Visitor(security), m_observer);

}

void Notifier::RaiseNewTradeEvent(const Trade &trade) {

	const class Visitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:		
		explicit Visitor(const Trade &trade)
				: m_trade(trade) {
			//...//
		}
	public:
		void operator ()(const boost::shared_ptr<Strategy> &strategy) const {
			AssertEq(
				m_trade.security->GetFullSymbol(),
				strategy->GetSecurity().GetFullSymbol());
			strategy->RaiseNewTradeEvent(
				m_trade.time,
				m_trade.price,
				m_trade.qty,
				m_trade.side);
		}
		void operator ()(const boost::shared_ptr<Service> &service) const {
			AssertEq(
				m_trade.security->GetFullSymbol(),
				service->GetSecurity().GetFullSymbol());
			if (	service->RaiseNewTradeEvent(
						m_trade.time,
						m_trade.price,
						m_trade.qty,
						m_trade.side)) {
				NotifyServiceDataUpdateSubscribers(*service);
			}
		}
		void operator ()(const boost::shared_ptr<Observer> &observer) const {
#			ifdef DEV_VER
			{
				bool isExists = false;
				foreach (const auto &securityPtr, observer->GetNotifyList()) {
					if (&*securityPtr == m_trade.security) {
						isExists = true;
						break;
					}
				}
				Assert(isExists);
			}
#			endif
			observer->RaiseNewTradeEvent(
				*m_trade.security,
				m_trade.time,
				m_trade.price,
				m_trade.qty,
				m_trade.side);
		}
	private:
		const Trade &m_trade;
	};

	boost::apply_visitor(Visitor(trade), m_observer);

}

void Notifier::RaisePositionUpdateEvent(Position &position) {

	const class Visitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:		
		explicit Visitor(Position &position)
				: m_position(position) {
			//...//
		}
	public:
		void operator ()(const boost::shared_ptr<Strategy> &strategy) const {
			AssertEq(
				m_position.GetSecurity().GetFullSymbol(),
				strategy->GetSecurity().GetFullSymbol());
			strategy->RaisePositionUpdateEvent(m_position);
		}
		void operator ()(const boost::shared_ptr<Service> &) const {
			throw LogicError(
				"Failed to raise position update event for service");
		}
		void operator ()(const boost::shared_ptr<Observer> &) const {
			throw LogicError(
				"Failed to raise position update event for observer");
		}
	private:
		Position &m_position;
	};

	boost::apply_visitor(Visitor(position), m_observer);

}

//////////////////////////////////////////////////////////////////////////
