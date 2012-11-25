/**************************************************************************
 *   Created: 2012/09/19 23:58:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Observer.hpp"
#include "Service.hpp"

using namespace Trader;
using namespace Trader::Lib;

//////////////////////////////////////////////////////////////////////////

class Observer::Implementation {

public:

	NotifyList m_notifyList;
	boost::shared_ptr<Trader::TradeSystem> tradeSystem;

public:

	explicit Implementation(
				const Observer::NotifyList &notifyList,
				boost::shared_ptr<Trader::TradeSystem> tradeSystem)
			: m_notifyList(notifyList),
			tradeSystem(tradeSystem) {
		//...//
	}

	~Implementation() {
		//...//
	}

};

//////////////////////////////////////////////////////////////////////////

Observer::Observer(
			const std::string &tag,
			const Observer::NotifyList &notifyList,
			boost::shared_ptr<Trader::TradeSystem> tradeSystem,
			boost::shared_ptr<const Settings> settings)
		: Module(tag, settings),
		m_pimpl(new Implementation(notifyList, tradeSystem)) {
	//...//
}

Observer::~Observer() {
	delete m_pimpl;
}

void Observer::OnNewTrade(
					const Trader::Security &,
					const boost::posix_time::ptime &,
					ScaledPrice,
					Qty,
					OrderSide) {
	Log::Error(
		"\"%1%\" subscribed to new trades, but can't work with it"
			" (hasn't implementation of OnNewTrade).",
		*this);
	throw MethodDoesNotImplementedError(
		"Module subscribed to new trades, but can't work with it");
}

void Observer::OnServiceDataUpdate(const Trader::Service &service) {
	Log::Error(
		"\"%1%\" subscribed to \"%2%\", but can't work with it"
			" (hasn't implementation of OnServiceDataUpdate).",
		*this,
		service);
 	throw MethodDoesNotImplementedError(
 		"Module subscribed to service, but can't work with it");
}

const std::string & Observer::GetTypeName() const {
	static const std::string typeName = "Observer";
	return typeName;
}

const Observer::NotifyList & Observer::GetNotifyList() const {
	return m_pimpl->m_notifyList;
}

TradeSystem & Observer::GetTradeSystem() {
	return *m_pimpl->tradeSystem;
}
