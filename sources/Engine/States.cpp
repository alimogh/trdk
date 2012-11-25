/**************************************************************************
 *   Created: 2012/11/22 11:46:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "States.hpp"
#include "Notifier.hpp"
#include "Core/PositionReporter.hpp"
#include "Core/Strategy.hpp"
#include "Core/Service.hpp"
#include "Core/Observer.hpp"
#include "Core/Settings.hpp"

namespace mi = boost::multi_index;
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
		template<typename Module>
		void operator ()(const boost::shared_ptr<Module> &module) const {
			const Module::Lock lock(module->GetMutex());
			module->OnServiceDataUpdate(m_service);
		}
		template<>
		void operator ()(const boost::shared_ptr<Strategy> &module) const {
			{
				const Module::Lock lock(module->GetMutex());
				if (!module->OnServiceDataUpdate(m_service)) {
					return;
				}
			}
			module->CheckPositions();
		}
		template<>
		void operator ()(const boost::shared_ptr<Service> &module) const {
			{
				const Module::Lock lock(module->GetMutex());
				if (!module->OnServiceDataUpdate(m_service)) {
					return;
				}
			}
			NotifyServiceDataUpdateSubscribers(*module);
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

	class NotifyVisitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	
	public:		
	
		explicit NotifyVisitor(const Trade &trade)
				: m_trade(trade) {
			//...//
		}
		
	public:

		void operator ()(const boost::shared_ptr<Strategy> &strategy) const {
			if (!Notify(*strategy)) {
				return;
			}
			strategy->CheckPositions();
		}

		void operator ()(const boost::shared_ptr<Service> &service) const {
			if (!Notify(*service)) {
				return;
			}
			NotifyServiceDataUpdateSubscribers(*service);
		}
	
		void operator ()(const boost::shared_ptr<Observer> &observer) const {
			Notify(*observer);
		}
	
	private:
	
		bool Notify(SecurityAlgo &observer) const {
			AssertEq(
				m_trade.security->GetFullSymbol(),
				observer.GetSecurity().GetFullSymbol());
			const Module::Lock lock(observer.GetMutex());
			return observer.OnNewTrade(
				m_trade.time,
				m_trade.price,
				m_trade.qty,
				m_trade.side);
		}

		void Notify(Observer &observer) const {
			const Module::Lock lock(observer.GetMutex());
			observer.OnNewTrade(
				*m_trade.security,
				m_trade.time,
				m_trade.price,
				m_trade.qty,
				m_trade.side);
		}

	private:
	
		const Trade &m_trade;
	
	};

}

//////////////////////////////////////////////////////////////////////////

Module & TradeObserverState::GetObserver() {
	return boost::apply_visitor(GetModuleVisitor(), m_observer);
}

void TradeObserverState::NotifyNewTrades(const Trade &trade) {
	boost::apply_visitor(NotifyVisitor(trade), m_observer);
}

//////////////////////////////////////////////////////////////////////////
