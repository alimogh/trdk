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
		STATE_NOT_CLOSED,
		STATE_NOT_OPENED
	};

}

//////////////////////////////////////////////////////////////////////////

Position::DynamicData::DynamicData()
		: orderId(0),
		price(0),
		qty(0) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

Position::Position(
			boost::shared_ptr<const Security> security,
			Type type,
			Qty qty,
			Price startPrice,
			Price decisionAks,
			Price decisionBid,
			Price takeProfit,
			Price stopLoss,
			AlgoFlag algoFlag)
		: m_security(security),
		m_type(type),
		m_planedQty(qty),
		m_startPrice(startPrice),
		m_decisionAks(decisionAks),
		m_decisionBid(decisionBid),
		m_takeProfit(takeProfit),
		m_stopLoss(stopLoss),
		m_closeType(CLOSE_TYPE_NONE),
		m_isReported(false),
		m_algoFlag(algoFlag) {
	Interlocking::Exchange(m_state, STATE_NONE);
}

Position::~Position() {
	//...//
}

Position::AlgoFlag Position::GetAlgoFlag() const {
	return m_algoFlag;
}

void Position::SetAlgoFlag(AlgoFlag algoFlag) {
	m_algoFlag = algoFlag;
}

bool Position::IsReported() const {
	Assert(IsClosed());
	return m_isReported;
}

void Position::MarkAsReported() {
	Assert(IsClosed());
	Assert(!m_isReported);
	m_isReported = true;
}

void Position::SetCloseType(CloseType closeType) {
	m_closeType = closeType;
}

Position::CloseType Position::GetCloseType() const {
	return m_closeType;
}

void Position::UpdateOpening(
			TradeSystem::OrderId orderId,
			TradeSystem::OrderStatus orderStatus,
			Qty filled,
			Qty remaining,
			Price avgPrice,
			Price /*lastPrice*/) {

	Assert(m_state < STATE_OPENED);
	Assert(m_opened.orderId == 0 || m_opened.orderId == orderId);
	Assert(m_closed.orderId == 0);
	Assert(m_closed.price == 0);
	Assert(m_opened.time.is_not_a_date_time());
	Assert(m_closed.time.is_not_a_date_time());
	Assert(m_opened.qty <= m_planedQty);
	Assert(m_closed.qty == 0);

	m_opened.orderId = orderId;

	State state = State(m_state);
	switch (orderStatus) {
		default:
			AssertFail("Unknown order status");
		case TradeSystem::ORDER_STATUS_PENDIGN:
		case TradeSystem::ORDER_STATUS_SUBMITTED:
			Assert(m_opened.qty == 0);
			Assert(m_opened.price == 0);
			return;
		case TradeSystem::ORDER_STATUS_FILLED:
			Assert(filled + remaining == m_planedQty);
			Assert(m_opened.qty + filled <= m_planedQty);
			Assert(avgPrice > 0);
			m_opened.price = avgPrice;
			m_opened.qty += filled;
			if (remaining == 0) {
				Assert(m_opened.qty > 0);
				state = STATE_OPENED;
			}
			break;
		case TradeSystem::ORDER_STATUS_INACTIVE:
		case TradeSystem::ORDER_STATUS_ERROR:
			Log::Error(
				"Position OPEN error: symbol: \"%1%\", trade system state: %2%, order ID: %3%, opened qty: %4%.",
				GetSecurity().GetFullSymbol(),
				orderStatus,
				m_opened.orderId,
				m_opened.qty);
		case TradeSystem::ORDER_STATUS_CANCELLED:
			state = m_opened.qty == 0
				?	STATE_NOT_OPENED
				:	STATE_OPENED;
			break;
	}
	Assert(m_opened.qty <= m_planedQty);

	if (state >= STATE_OPENED && state < STATE_NOT_CLOSED) {
		Assert(state == STATE_OPENED);
		Assert(m_opened.time.is_not_a_date_time());
		if (m_opened.time.is_not_a_date_time()) {
			m_opened.time = boost::get_system_time();
		}
	}

	Interlocking::Exchange(m_state, state);
	if (m_state >= STATE_OPENED) {
		Assert(m_opened.orderId != 0);
		Assert(m_closed.orderId == 0);
		Assert(m_closed.time.is_not_a_date_time());
		Assert(m_closed.qty == 0);
		Assert(m_closed.price == 0);
		m_stateUpdateSignal();
	}

}

void Position::UpdateClosing(
			TradeSystem::OrderId orderId,
			TradeSystem::OrderStatus orderStatus,
			Qty filled,
			Qty remaining,
			Price avgPrice,
			Price /*lastPrice*/) {

	Assert(m_state == STATE_OPENED);
	Assert(m_opened.orderId != 0);
	Assert(m_closed.orderId == 0 || m_closed.orderId == orderId);
	Assert(m_opened.price != 0);
	Assert(!m_opened.time.is_not_a_date_time());
	Assert(m_closed.time.is_not_a_date_time());
	Assert(m_opened.qty <= m_planedQty);
	Assert(m_closed.qty <= m_opened.qty);

	m_closed.orderId = orderId;

	State state = State(m_state);
	switch (orderStatus) {
		default:
			AssertFail("Unknown order status");
		case TradeSystem::ORDER_STATUS_PENDIGN:
		case TradeSystem::ORDER_STATUS_SUBMITTED:
			Assert(m_closed.qty == 0);
			Assert(m_closed.price == 0);
			return;
		case TradeSystem::ORDER_STATUS_FILLED:
			Assert(filled + remaining == m_opened.qty);
			Assert(m_closed.qty + filled <= m_opened.qty);
			Assert(avgPrice > 0);
			m_closed.price = avgPrice;
			m_closed.qty += filled;
			if (remaining == 0) {
				Assert(m_closed.qty > 0);
				state = STATE_CLOSED;
			}
			break;
		case TradeSystem::ORDER_STATUS_INACTIVE:
		case TradeSystem::ORDER_STATUS_ERROR:
		case TradeSystem::ORDER_STATUS_CANCELLED:
			state = STATE_NOT_CLOSED;
			Log::Error(
				"Position CLOSE error: symbol:"
					" \"%1%\", trade system state: %2%, orders ID: %3%->%4%, qty: %5%->%6%.",
				GetSecurity().GetFullSymbol(),
				orderStatus,
				m_opened.orderId,
				m_closed.orderId,
				m_opened.qty,
				m_closed.qty);
			break;
	}
	Assert(m_opened.qty <= m_planedQty);

	if (state > STATE_OPENED && state < STATE_NOT_CLOSED) {
		Assert(m_closed.time.is_not_a_date_time());
		if (m_closed.time.is_not_a_date_time()) {
			m_closed.time = boost::get_system_time();
		}
	}

	Interlocking::Exchange(m_state, state);
	if (m_state > STATE_OPENED) {
		Assert(m_opened.orderId != 0);
		Assert(!m_opened.time.is_not_a_date_time());
		Assert(m_closed.orderId != 0);
		Assert(!m_closed.time.is_not_a_date_time());
		m_stateUpdateSignal();
	}

}

TradeSystem::OrderId Position::GetOpenOrderId() const {
	return m_opened.orderId;
}

TradeSystem::OrderId Position::GetCloseOrderId() const {
	return m_closed.orderId;
}

Position::Time Position::GetOpenTime() const {
	return m_closed.time;
}

Position::Time Position::GetCloseTime() const {
	return m_opened.time;
}

const Security & Position::GetSecurity() const {
	return *m_security;
}

bool Position::IsOpened() const {
	return m_state >= STATE_OPENED && !IsNotOpened();
}

bool Position::IsNotOpened() const {
	return m_state == STATE_NOT_OPENED;
}

bool Position::IsClosed() const {
	return m_state == STATE_CLOSED && IsNotClosed();
}

bool Position::IsNotClosed() const {
	return m_state == STATE_NOT_CLOSED;
}

bool Position::IsCompleted() const {
	return m_state >= STATE_CLOSED;
}

Position::StateUpdateConnection Position::Subscribe(
			const StateUpdateSlot &slot)
		const {
	return StateUpdateConnection(m_stateUpdateSignal.connect(slot));
}

Security::OrderStatusUpdateSlot Position::GetSellOrderStatusUpdateSlot() {
	switch (m_type) {
		case TYPE_LONG:
			return boost::bind(&Position::UpdateClosing, shared_from_this(), _1, _2, _3, _4, _5, _6);
		case  TYPE_SHORT:
			return boost::bind(&Position::UpdateOpening, shared_from_this(), _1, _2, _3, _4, _5, _6);
		default:
			AssertFail("Unknown position type.");
			throw Exception("Unknown position type");
	}
}

Security::OrderStatusUpdateSlot Position::GetBuyOrderStatusUpdateSlot() {
	switch (m_type) {
		case TYPE_LONG:
			return boost::bind(&Position::UpdateOpening, shared_from_this(), _1, _2, _3, _4, _5, _6);
		case  TYPE_SHORT:
			return boost::bind(&Position::UpdateClosing, shared_from_this(), _1, _2, _3, _4, _5, _6);
		default:
			AssertFail("Unknown position type.");
			throw Exception("Unknown position type");
	}
}

Position::Type Position::GetType() const {
	return m_type;
}

const char * Position::GetTypeStr() const {
	switch (m_type) {
		case TYPE_LONG:
			return "long";
		case TYPE_SHORT:
			return "short";
		default:
			AssertFail("Unknown position type");
			throw Exception("Unknown position type");
	}
}

Position::Qty Position::GetPlanedQty() const {
	return m_planedQty;
}

Position::Qty Position::GetOpenedQty() const {
	return m_opened.qty;
}

Position::Qty Position::GetClosedQty() const {
	return m_closed.qty;
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
	return m_opened.price;
}

Position::Price Position::GetClosePrice() const {
	return m_closed.price;
}
