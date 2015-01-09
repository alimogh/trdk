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

	void RaiseServiceDataUpdateEvent(
			Service &,
			const TimeMeasurement::Milestones &);

	class RaiseServiceDataUpdateEventVisitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:
		explicit RaiseServiceDataUpdateEventVisitor(
				const Service &service,
				const TimeMeasurement::Milestones &timeMeasurement)
			: m_service(service),
			m_timeMeasurement(timeMeasurement) {
			//...//
		}
	public:		
		void operator ()(Strategy &strategy) const {
			strategy.RaiseServiceDataUpdateEvent(m_service, m_timeMeasurement);
		}
		void operator ()(Service &service) const {
			if (
					!service.RaiseServiceDataUpdateEvent(
						m_service,
						m_timeMeasurement)) {
				return;
			}
			RaiseServiceDataUpdateEvent(service, m_timeMeasurement);
		}
		void operator ()(Observer &observer) const {
			observer.RaiseServiceDataUpdateEvent(m_service, m_timeMeasurement);
		}
	private:
		const Service &m_service;
		const TimeMeasurement::Milestones &m_timeMeasurement;
	};

	void RaiseServiceDataUpdateEvent(
			Service &service,
			const TimeMeasurement::Milestones &timeMeasurement) {
		const RaiseServiceDataUpdateEventVisitor visitor(
			service,
			timeMeasurement);
		foreach (auto subscriber, service.GetSubscribers()) {
			try {
				boost::apply_visitor(visitor, subscriber);
			} catch (const Exception &ex) {
				service.GetContext().GetLog().Error(
					"Error at service subscribers notification:"
						" \"%1%\" (service: \"%2%\", subscriber: \"%3%\").",
					ex,
					service,
					boost::apply_visitor(Visitors::GetModule(), subscriber));
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
	};
	template<>
	bool AvailabilityCheckVisitor::operator ()(
			const Strategy &strategy)
			const {
		return !strategy.IsBlocked();
	}

	class BlockVisitor : public boost::static_visitor<void> {
	public:
		template<typename Module>
		void operator ()(Module &) const throw() {
			//...//
		}
	};
	template<>
	void BlockVisitor::operator ()(Strategy &strategy) const throw() {
		strategy.Block();
	}

	//////////////////////////////////////////////////////////////////////////

}

//////////////////////////////////////////////////////////////////////////

const Module & SubscriberPtrWrapper::operator *() const {
	return boost::apply_visitor(Visitors::GetConstModule(), m_subscriber);
}

const Module * SubscriberPtrWrapper::operator ->() const {
	return &**this;
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
		Security &security,
		const TimeMeasurement::Milestones &timeMeasurement)
		const {

	class Visitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:		
		explicit Visitor(
					Security &security,
					const TimeMeasurement::Milestones &timeMeasurement)
			: m_security(security),
			m_timeMeasurement(timeMeasurement) {
			//...//
		}
	public:
		void operator ()(Strategy &strategy) const {
			strategy.RaiseLevel1UpdateEvent(m_security, m_timeMeasurement);
		}
		void operator ()(Service &service) const {
			if (service.RaiseLevel1UpdateEvent(m_security)) {
				RaiseServiceDataUpdateEvent(service, m_timeMeasurement);
			}
		}
		void operator ()(Observer &observer) const {
			observer.RaiseLevel1UpdateEvent(m_security);
		}
	private:
		Security &m_security;
		const TimeMeasurement::Milestones &m_timeMeasurement;
	};

	boost::apply_visitor(Visitor(security, timeMeasurement), m_subscriber);

}

void SubscriberPtrWrapper::RaiseLevel1TickEvent(
		const Level1Tick &tick,
		const TimeMeasurement::Milestones &timeMeasurement)
		const {

	class Visitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:		
		explicit Visitor(
				const Level1Tick &tick,
				const TimeMeasurement::Milestones &timeMeasurement)
			: m_tick(tick),
			m_timeMeasurement(timeMeasurement) {
			//...//
		}
	public:
		void operator ()(Strategy &strategy) const {
			strategy.RaiseLevel1TickEvent(
				*m_tick.security,
				m_tick.time,
				m_tick.value);
		}
		void operator ()(Service &service) const {
			if (	
					service.RaiseLevel1TickEvent(
						*m_tick.security,
						m_tick.time,
						m_tick.value)) {
				RaiseServiceDataUpdateEvent(service, m_timeMeasurement);
			}
		}
		void operator ()(Observer &observer) const {
			observer.RaiseLevel1TickEvent(
				*m_tick.security,
				m_tick.time,
				m_tick.value);
		}
	private:
		const Level1Tick &m_tick;
		const TimeMeasurement::Milestones &m_timeMeasurement;
	};

	boost::apply_visitor(Visitor(tick, timeMeasurement), m_subscriber);

}

void SubscriberPtrWrapper::RaiseNewTradeEvent(
		const Trade &trade,
		const TimeMeasurement::Milestones &timeMeasurement)
		const {

	class Visitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:		
		explicit Visitor(
				const Trade &trade,
				const TimeMeasurement::Milestones &timeMeasurement)
			: m_trade(trade),
			m_timeMeasurement(timeMeasurement) {
			//...//
		}
	public:
		void operator ()(Strategy &strategy) const {
			strategy.RaiseNewTradeEvent(
				*m_trade.security,
				m_trade.time,
				m_trade.price,
				m_trade.qty,
				m_trade.side);
		}
		void operator ()(Service &service) const {
			if (	
					service.RaiseNewTradeEvent(
						*m_trade.security,
						m_trade.time,
						m_trade.price,
						m_trade.qty,
						m_trade.side)) {
				RaiseServiceDataUpdateEvent(service, m_timeMeasurement);
			}
		}
		void operator ()(Observer &observer) const {
			observer.RaiseNewTradeEvent(
				*m_trade.security,
				m_trade.time,
				m_trade.price,
				m_trade.qty,
				m_trade.side);
		}
	private:
		const Trade &m_trade;
		const TimeMeasurement::Milestones &m_timeMeasurement;
	};

	boost::apply_visitor(Visitor(trade, timeMeasurement), m_subscriber);

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

void SubscriberPtrWrapper::RaiseBrokerPositionUpdateEvent(
		const BrokerPosition &position,
		const TimeMeasurement::Milestones &timeMeasurement)
		const {

	class Visitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:		
		explicit Visitor(
				const BrokerPosition &position,
				const TimeMeasurement::Milestones &timeMeasurement)
			: m_position(position),
			m_timeMeasurement(timeMeasurement) {
			//...//
		}
	public:
		void operator ()(Strategy &strategy) const {
			return strategy.RaiseBrokerPositionUpdateEvent(
				*m_position.security,
				m_position.qty,
				m_position.isInitial);
		}
		void operator ()(Service &service) const {
			if (	
					service.RaiseBrokerPositionUpdateEvent(
						*m_position.security,
						m_position.qty,
						m_position.isInitial)) {
				RaiseServiceDataUpdateEvent(service, m_timeMeasurement);
			}
		}
		void operator ()(Observer &observer) const {
			return observer.RaiseBrokerPositionUpdateEvent(
				*m_position.security,
				m_position.qty,
				m_position.isInitial);
		}
	private:
		const BrokerPosition &m_position;
		const TimeMeasurement::Milestones &m_timeMeasurement;
	};

	boost::apply_visitor(Visitor(position, timeMeasurement), m_subscriber);

}

void SubscriberPtrWrapper::RaiseNewBarEvent(
		Security &security,
		const Security::Bar &bar,
		const TimeMeasurement::Milestones &timeMeasurement)
		const {

	class Visitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:		
		explicit Visitor(
				Security &security,
				const Security::Bar &bar,
				const TimeMeasurement::Milestones &timeMeasurement)
			: m_source(security),
			m_bar(bar),
			m_timeMeasurement(timeMeasurement) {
			//...//
		}
	public:
		void operator ()(Strategy &strategy) const {
			strategy.RaiseNewBarEvent(m_source, m_bar);
		}
		void operator ()(Service &service) const {
			if (service.RaiseNewBarEvent(m_source, m_bar)) {
				RaiseServiceDataUpdateEvent(service, m_timeMeasurement);
			}
		}
		void operator ()(Observer &observer) const {
			observer.RaiseNewBarEvent(m_source, m_bar);
		}
	private:
		Security &m_source;
		const Security::Bar &m_bar;
		const TimeMeasurement::Milestones &m_timeMeasurement;
	};

	boost::apply_visitor(
		Visitor(security, bar, timeMeasurement),
		m_subscriber);

}

void SubscriberPtrWrapper::RaiseBookUpdateTickEvent(
		Security &security,
		const Security::Book &book,
		const TimeMeasurement::Milestones &timeMeasurement)
		const {
	
	class Visitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:		
		explicit Visitor(
				Security &security,
				const Security::Book &book,
				const TimeMeasurement::Milestones &timeMeasurement)
			: m_source(security),
			m_book(book),
			m_timeMeasurement(timeMeasurement) {
			//...//
		}
	public:
		void operator ()(Strategy &strategy) const {
			strategy.RaiseBookUpdateTickEvent(
				m_source,
				m_book,
				m_timeMeasurement);
		}
		void operator ()(Service &service) const {
			if (
					service.RaiseBookUpdateTickEvent(
						m_source,
						m_book,
						m_timeMeasurement)) {
				RaiseServiceDataUpdateEvent(service, m_timeMeasurement);
			}
		}
		void operator ()(Observer &) const {
			AssertFail("Not supported.");
		}
	private:
		Security &m_source;
		const Security::Book &m_book;
		const TimeMeasurement::Milestones &m_timeMeasurement;
	};

	boost::apply_visitor(
		Visitor(security, book, timeMeasurement),
		m_subscriber);

}

//////////////////////////////////////////////////////////////////////////
