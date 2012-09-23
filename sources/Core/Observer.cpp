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

public:

	explicit Implementation(const Observer::NotifyList &notifyList)
			: m_notifyList(notifyList) {
		//...//
	}

	~Implementation() {
		//...//
	}

};

//////////////////////////////////////////////////////////////////////////

Observer::Observer(
			const std::string &tag,
			const Observer::NotifyList &notifyList)
		: Module(tag),
		m_pimpl(new Implementation(notifyList)) {
	//...//
}

Observer::~Observer() {
	delete m_pimpl;
}

const Observer::NotifyList & Observer::GetNotifyList() const {
	return m_pimpl->m_notifyList;
}
