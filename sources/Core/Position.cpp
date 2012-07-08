/**************************************************************************
 *   Created: 2012/07/08 04:07:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Position.hpp"

//////////////////////////////////////////////////////////////////////////

namespace {

	enum State {
		STATE_NONE,
		STATE_OPENED,
		STATE_CLOSED,
		STATE_CANCELED
	};

}

//////////////////////////////////////////////////////////////////////////

Position::Position(
			Type type,
			Price startPrice,
			Price decisionAks,
			Price decisionBid,
			Price takeProfit,
			Price stopLoss)
		: m_type(type),
		m_startPrice(startPrice),
		m_decisionAks(decisionAks),
		m_decisionBid(decisionBid),
		m_takeProfit(takeProfit),
		m_stopLoss(stopLoss) {
	Interlocking::Exchange(m_openPrice, 0);
	Interlocking::Exchange(m_closePrice, 0);
	Interlocking::Exchange(m_state, STATE_NONE);
}

Position::~Position() {
	//...//
}

bool Position::IsOpened() const {
	return m_state == STATE_OPENED;
}

bool Position::IsClosed() const {
	return m_state == STATE_CLOSED;
}

bool Position::IsCanceled() const {
	return m_state == STATE_CANCELED;
}

bool Position::IsCompleted() const {
	return IsCanceled() || IsClosed();
}

Position::StateUpdateConnection Position::Subscribe(
			const StateUpdateSlot &slot)
		const {
	return StateUpdateConnection(m_stateUpdateSignal.connect(slot));
}

Position::Type Position::GetType() const {
	return m_type;
}

Position::Price Position::GetStartPrice() const {
	return m_startPrice;
}

Position::Price Position::GetDecisionAks() const {
	return m_decisionAks;
}

Position::Price Position::GetDecisionBid() const {
	return m_decisionBid;
}

Position::Price Position::GetTakeProfit() const {
	return m_takeProfit;
}

Position::Price Position::GetStopLoss() const {
	return m_stopLoss;
}

Position::Price Position::GetOpenPrice() const {
	return m_openPrice;
}

Position::Price Position::GetClosePrice() const {
	return m_closePrice;
}
