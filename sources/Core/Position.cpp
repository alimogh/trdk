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
		//! not opened
		STATE_NONE,
		//! partial filled
		STATE_OPENING,
		//! not opened, error occurred
		STATE_OPEN_ERROR,
		//! opened
		STATE_OPENED,
		//! partial filled
		STATE_CLOSING,
		//! closing after order cancel
		STATE_RECLOSING,
		//! opened and not closed, error occurred at closing
		STATE_CLOSE_ERROR,
		//! opened and then closed
		STATE_CLOSED
	};

}

//////////////////////////////////////////////////////////////////////////

Position::DynamicData::DynamicData()
		: orderId(0),
		price(0),
		qty(0),
		comission(0) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

Position::Position(
			boost::shared_ptr<const DynamicSecurity> security,
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
	return m_isReported;
}

void Position::MarkAsReported() {
	Assert(!m_isReported);
	m_isReported = true;
}

void Position::SetCloseType(CloseType closeType) {
	m_closeType = closeType;
}

Position::CloseType Position::GetCloseType() const {
	return m_closeType;
}

const char * Position::GetCloseTypeStr() const {
	switch (m_closeType) {
		default:
			AssertFail("Unknown position close type.");
		case CLOSE_TYPE_NONE:
			return "-";
		case CLOSE_TYPE_TAKE_PROFIT:
			return "t/p";
		case CLOSE_TYPE_STOP_LOSS:
			return "s/l";
	}
}

void Position::UpdateOpening(
			TradeSystem::OrderId orderId,
			TradeSystem::OrderStatus orderStatus,
			Qty filled,
			Qty remaining,
			double avgPrice,
			double /*lastPrice*/) {

	Assert(!IsOpened());
	Assert(m_state == STATE_NONE || m_state == STATE_OPENING);
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
			m_opened.price = m_security->Scale(avgPrice);
			m_opened.qty += filled;
			Assert(m_opened.qty > 0);
			state = remaining == 0 ? STATE_OPENED : STATE_OPENING;
			ReportOpeningUpdate("filled", orderStatus, state);
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
				?	STATE_OPEN_ERROR
				:	STATE_OPENED;
			if (m_opened.qty > 0) {
				ReportOpeningUpdate("canceled", orderStatus, state);
			}
			break;
	}
	Assert(m_opened.qty <= m_planedQty);

	Assert(m_opened.time.is_not_a_date_time());
	if (state == STATE_OPENED) {
		m_opened.time = boost::get_system_time();
	}

	if (Interlocking::Exchange(m_state, state) != state) {
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
			double avgPrice,
			double /*lastPrice*/) {

	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(m_state == STATE_OPENED || m_state == STATE_RECLOSING || m_state == STATE_CLOSING);
	Assert(m_opened.orderId != 0);
	Assert(m_state == STATE_RECLOSING || (m_closed.orderId == 0 || m_closed.orderId == orderId));
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
			m_closed.price = m_security->Scale(avgPrice);
			m_closed.qty += filled;
			Assert(m_closed.qty > 0);
			state = remaining == 0 ? STATE_CLOSED : STATE_CLOSING;
			ReportClosingUpdate("filled", orderStatus, state);
			break;
		case TradeSystem::ORDER_STATUS_INACTIVE:
		case TradeSystem::ORDER_STATUS_ERROR:
			state = STATE_CLOSE_ERROR;
			Log::Error(
				"Position CLOSE error: symbol:"
					" \"%1%\", trade system state: %2%, orders ID: %3%->%4%, qty: %5%->%6%.",
				GetSecurity().GetFullSymbol(),
				orderStatus,
				m_opened.orderId,
				m_closed.orderId,	
				m_opened.qty,
				m_closed.qty);
			ReportClosingUpdate("error", orderStatus, state);
			break;
		case TradeSystem::ORDER_STATUS_CANCELLED:
			state = STATE_RECLOSING;
			if (m_closed.qty > 0) {
				ReportClosingUpdate("canceled", orderStatus, state);
			}
			break;
	}
	Assert(m_opened.qty <= m_planedQty);

	Assert(m_closed.time.is_not_a_date_time());
	if (state == STATE_CLOSED) {
		m_closed.time = boost::get_system_time();
	}

	if (Interlocking::Exchange(m_state, state) != state) {
		Assert(m_opened.orderId != 0);
		Assert(!m_opened.time.is_not_a_date_time());
		Assert(m_closed.orderId != 0);
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

Position::Price Position::GetCommission() const {
	return m_opened.comission + m_closed.comission;
}

const DynamicSecurity & Position::GetSecurity() const {
	return *m_security;
}

bool Position::IsOpened() const {
	return m_state >= STATE_OPENED;
}

bool Position::IsOpenError() const {
	return m_state == STATE_OPEN_ERROR;
}

bool Position::IsClosed() const {
	return m_state >= STATE_CLOSED;
}

bool Position::IsCloseError() const {
	return m_state == STATE_CLOSED;
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

void Position::ReportOpeningUpdate(
			const char *eventDesc,
			TradeSystem::OrderStatus orderStatus,
			long state)
		const {
	Log::Trading(
		"position",
		"%1% %2% open-%3% qty=%4%->%5% price=%6%->%7% order-id=%8%"
			" order-status=%9% state=%10% cur-ask-bid=%11%/%12%"
			" take-profit=%13% stop-loss=%14%",
		GetSecurity().GetSymbol(),
		GetTypeStr(),
		eventDesc,
		GetPlanedQty(),
		GetOpenedQty(),
		GetSecurity().Descale(GetStartPrice()),
		GetSecurity().Descale(GetOpenPrice()),
		GetOpenOrderId(),
		GetSecurity().GetTradeSystem().GetStringStatus(orderStatus),
		state,
		GetSecurity().GetAsk(),
		GetSecurity().GetBid(),
		GetSecurity().Descale(GetTakeProfit()),
		GetSecurity().Descale(GetStopLoss()));
}

void Position::ReportClosingUpdate(
			const char *eventDesc,
			TradeSystem::OrderStatus orderStatus,
			long state)
		const {
	Log::Trading(
		"position",
		"%1% %2% close-%3% qty=%4%->%5% price=%6% order-id=%7%->%8%"
			" order-status=%9% state=%10% cur-ask-bid=%11%/%12%"
			" take-profit=%13% stop-loss=%14%",
		GetSecurity().GetSymbol(),
		GetTypeStr(),
		eventDesc,
		GetOpenedQty(),
		GetClosedQty(),
		GetSecurity().Descale(GetClosePrice()),
		GetOpenOrderId(),
		GetCloseOrderId(),
		GetSecurity().GetTradeSystem().GetStringStatus(orderStatus),
		state,
		GetSecurity().GetAsk(),
		GetSecurity().GetBid(),
		GetSecurity().Descale(GetTakeProfit()),
		GetSecurity().Descale(GetStopLoss()));
}
