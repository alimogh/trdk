/**************************************************************************
 *   Created: 2012/09/19 23:58:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Observer.hpp"

using namespace Trader;

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
			boost::shared_ptr<Trader::TradeSystem> tradeSystem)
		: Module(tag),
		m_pimpl(new Implementation(notifyList, tradeSystem)) {
	//...//
}

Observer::~Observer() {
	delete m_pimpl;
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
