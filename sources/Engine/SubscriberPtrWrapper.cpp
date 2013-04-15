/**************************************************************************
 *   Created: 2012/11/22 11:46:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "SubscriberPtrWrapper.hpp"
#include "Core/Strategy.hpp"
#include "Core/Service.hpp"
#include "Core/Observer.hpp"
#include "Core/Position.hpp"

namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Engine;

//////////////////////////////////////////////////////////////////////////

namespace {

	//////////////////////////////////////////////////////////////////////////

	void RaiseServiceDataUpdateEvent(Service &);

	class RaiseServiceDataUpdateEventVisitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:
		explicit RaiseServiceDataUpdateEventVisitor(const Service &service)
				: m_service(service) {
			//...//
		}
	public:		
		void operator ()(Strategy &strategy) const {
			strategy.RaiseServiceDataUpdateEvent(m_service);
		}
		void operator ()(Service &service) const {
			if (!service.RaiseServiceDataUpdateEvent(m_service)) {
				return;
			}
			RaiseServiceDataUpdateEvent(service);
		}
		void operator ()(Observer &observer) const {
			observer.RaiseServiceDataUpdateEvent(m_service);
		}
	private:
		const Service &m_service;
	};

	void RaiseServiceDataUpdateEvent(Service &service) {
		const RaiseServiceDataUpdateEventVisitor visitor(service);
		foreach (auto subscriber, service.GetSubscribers()) {
			try {
				boost::apply_visitor(visitor, subscriber);
			} catch (const Exception &ex) {
				service.GetContext().GetLog().Error(
					"Error at service subscribers notification:"
						" \"%1%\" (service: \"%2%\", subscriber: \"%3%\").",
					boost::make_tuple(
						boost::cref(ex),
						boost::cref(service),
						boost::cref(
							boost::apply_visitor(
								Visitors::GetModule(),
								subscriber))));
				throw;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////

	class AvailabilityCheckVisitor : public boost::static_visitor<bool> {
	public:
		template<typename Module>
		bool operator ()(const Module &) const {
			return true;
		}
		template<>
		bool operator ()(const Strategy &strategy) const {
			return !strategy.IsBlocked();
		}
	};

	class BlockVisitor : public boost::static_visitor<void> {
	public:
		template<typename Module>
		void operator ()(Module &) const throw() {
			//...//
		}
		template<>
		void operator ()(Strategy &strategy) const throw() {
			strategy.Block();
		}
	};

	//////////////////////////////////////////////////////////////////////////

}

//////////////////////////////////////////////////////////////////////////

const Module & SubscriberPtrWrapper::operator *() const {
	return boost::apply_visitor(Visitors::GetConstModule(), m_subscriber);
}

const Module * SubscriberPtrWrapper::operator ->() const {
	return &**this;
}

bool SubscriberPtrWrapper::operator <(const SubscriberPtrWrapper &rhs) const {
	return &**this < &*rhs;
}

bool SubscriberPtrWrapper::operator ==(const SubscriberPtrWrapper &rhs) const {
	return &**this == &*rhs;
}

bool SubscriberPtrWrapper::IsBlocked() const {
	return !boost::apply_visitor(AvailabilityCheckVisitor(), m_subscriber);
}

void SubscriberPtrWrapper::Block() const throw() {
	boost::apply_visitor(BlockVisitor(), m_subscriber);
}

void SubscriberPtrWrapper::RaiseLevel1UpdateEvent(
			const Security &security)
		const {

	const class Visitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:		
		explicit Visitor(const Security &security)
				: m_security(security) {
			//...//
		}
	public:
		void operator ()(Strategy &strategy) const {
			AssertEq(
				m_security.GetFullSymbol(),
				strategy.GetSecurity().GetFullSymbol());
			strategy.RaiseLevel1UpdateEvent(m_security);
		}
		void operator ()(Service &service) const {
			AssertEq(
				m_security.GetFullSymbol(),
				service.GetSecurity().GetFullSymbol());
			if (service.RaiseLevel1UpdateEvent(m_security)) {
				RaiseServiceDataUpdateEvent(service);
			}
		}
		void operator ()(Observer &observer) const {
#			ifdef DEV_VER
			{
				bool isExists = false;
				foreach (const auto &securityPtr, observer.GetNotifyList()) {
					if (&*securityPtr == &m_security) {
						isExists = true;
						break;
					}
				}
				Assert(isExists);
			}
#			endif
			observer.RaiseLevel1UpdateEvent(m_security);
		}
	private:
		const Security &m_security;
	};

	boost::apply_visitor(Visitor(security), m_subscriber);

}

void SubscriberPtrWrapper::RaiseNewTradeEvent(const Trade &trade) const {

	const class Visitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:		
		explicit Visitor(const Trade &trade)
				: m_trade(trade) {
			//...//
		}
	public:
		void operator ()(Strategy &strategy) const {
			AssertEq(
				m_trade.security->GetFullSymbol(),
				strategy.GetSecurity().GetFullSymbol());
			strategy.RaiseNewTradeEvent(
				*m_trade.security,
				m_trade.time,
				m_trade.price,
				m_trade.qty,
				m_trade.side);
		}
		void operator ()(Service &service) const {
			AssertEq(
				m_trade.security->GetFullSymbol(),
				service.GetSecurity().GetFullSymbol());
			if (	service.RaiseNewTradeEvent(
						*m_trade.security,
						m_trade.time,
						m_trade.price,
						m_trade.qty,
						m_trade.side)) {
				RaiseServiceDataUpdateEvent(service);
			}
		}
		void operator ()(Observer &observer) const {
#			ifdef DEV_VER
			{
				bool isExists = false;
				foreach (const auto &securityPtr, observer.GetNotifyList()) {
					if (&*securityPtr == m_trade.security) {
						isExists = true;
						break;
					}
				}
				Assert(isExists);
			}
#			endif
			observer.RaiseNewTradeEvent(
				*m_trade.security,
				m_trade.time,
				m_trade.price,
				m_trade.qty,
				m_trade.side);
		}
	private:
		const Trade &m_trade;
	};

	boost::apply_visitor(Visitor(trade), m_subscriber);

}

void SubscriberPtrWrapper::RaisePositionUpdateEvent(Position &position) const {

	class Visitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:		
		explicit Visitor(Position &position)
				: m_position(position) {
			//...//
		}
	public:
		void operator ()(Strategy &strategy) const {
			AssertEq(
				m_position.GetSecurity().GetFullSymbol(),
				strategy.GetSecurity().GetFullSymbol());
			strategy.RaisePositionUpdateEvent(m_position);
		}
		void operator ()(const Service &) const {
			throw Exception(
				"Internal error:"
					" Failed to raise position update event for service");
		}
		void operator ()(const Observer &) const {
			throw Exception(
				"Internal error:"
					" Failed to raise position update event for observer");
		}
	private:
		Position &m_position;
	};

	boost::apply_visitor(Visitor(position), m_subscriber);

}

//////////////////////////////////////////////////////////////////////////
