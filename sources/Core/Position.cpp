/**************************************************************************
 *   Created: 2012/07/08 04:07:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Position.hpp"
#include "Algo.hpp"
#include "Settings.hpp"

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
			boost::shared_ptr<Security> security,
			Qty qty,
			Price startPrice,
			boost::shared_ptr<const Algo> algo,
			boost::shared_ptr<AlgoPositionState> state /*= boost::shared_ptr<AlgoPositionState>()*/)
		: m_security(security),
		m_planedQty(qty),
		m_openStartPrice(startPrice),
		m_closeStartPrice(0),
		m_closeType(CLOSE_TYPE_NONE),
		m_isReported(false),
		m_algo(algo),
		m_algoState(state) {
	Interlocking::Exchange(m_state, STATE_NONE);
}

Position::~Position() {
	//...//
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
		case CLOSE_TYPE_TIME:
			return "time";
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

	Interlocking::Exchange(m_opened.orderId, orderId);

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
				"Position OPEN error: symbol: \"%1%\", algo: %2%"
					", trade system state: %3%, order ID: %4%, opened qty: %5%.",
				GetSecurity().GetFullSymbol(),
				m_algo->GetTag(),
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
		m_opened.time = !m_security->GetSettings().IsReplayMode()
			?	boost::get_system_time()
			:	m_security->GetLastMarketDataTime();
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
	Assert(m_opened.price != 0);
	Assert(!m_opened.time.is_not_a_date_time());
	Assert(m_closed.time.is_not_a_date_time());
	Assert(m_opened.qty <= m_planedQty);
	Assert(m_closed.qty <= m_opened.qty);

	if (m_closed.orderId != orderId) {
		if (m_closed.qty > 0 && m_closed.orderId) {
			Assert(m_state == STATE_OPENED);
			ReportCloseOrderChange(
				orderStatus,
				m_state,
				TradeSystem::OrderId(m_closed.orderId),
				orderId);
		}
		Interlocking::Exchange(m_closed.orderId, orderId);
	}

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
				"Position CLOSE error: symbol: \"%1%\", algo %2%"
					", trade system state: %3%, orders ID: %4%->%5%, qty: %6%->%7%.",
				GetSecurity().GetFullSymbol(),
				m_algo->GetTag(),
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
		m_closed.time = !m_security->GetSettings().IsReplayMode()
			?	boost::get_system_time()
			:	m_security->GetLastMarketDataTime();
	}

	if (Interlocking::Exchange(m_state, state) != state) {
		Assert(m_opened.orderId != 0);
		Assert(!m_opened.time.is_not_a_date_time());
		Assert(m_closed.orderId != 0);
		m_stateUpdateSignal();
	}

}

TradeSystem::OrderId Position::GetOpenOrderId() const {
	return TradeSystem::OrderId(m_opened.orderId);
}

TradeSystem::OrderId Position::GetCloseOrderId() const {
	return TradeSystem::OrderId(m_closed.orderId);
}

Position::Time Position::GetOpenTime() const {
	return m_opened.time;
}

Position::Time Position::GetCloseTime() const {
	return m_closed.time;
}

Position::Price Position::GetCommission() const {
	return m_opened.qty * GetSecurity().Scale(.01); // m_opened.comission + m_closed.comission;
}

const Security & Position::GetSecurity() const {
	return const_cast<Position *>(this)->GetSecurity();
}

Security & Position::GetSecurity() {
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

bool Position::IsCloseCanceled() const {
	return m_state == STATE_RECLOSING;
}

void Position::ResetState() {
	switch (m_state) {
		case STATE_NONE:
		case STATE_OPENING:
		case STATE_OPEN_ERROR:
			Interlocking::Exchange(m_state, STATE_NONE);
			break;
		case STATE_OPENED:
			break;
		case STATE_CLOSING:
		case STATE_RECLOSING:
		case STATE_CLOSE_ERROR:
			Interlocking::Exchange(m_state, STATE_OPENED);
			break;
		case STATE_CLOSED:
			throw Exception("Couldn't reset position state");
			break;
		default:
			AssertFail("Unknown position state.");
	}
}

Position::StateUpdateConnection Position::Subscribe(
			const StateUpdateSlot &slot)
		const {
	return StateUpdateConnection(m_stateUpdateSignal.connect(slot));
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

Position::Price Position::GetOpenStartPrice() const {
	return m_openStartPrice;
}

Position::Price Position::GetOpenPrice() const {
	return m_opened.price;
}

Position::Price Position::GetCloseStartPrice() const {
	return m_closeStartPrice;
}

void Position::SetCloseStartPrice(Position::Price price) {
	m_closeStartPrice = price;
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
		"%1% %2% open-%3% %4% qty=%5%->%6% price=%7%->%8% order-id=%9%"
			" order-status=%10% state=%11% cur-ask-bid=%12%/%13%",
		GetSecurity().GetSymbol(),
		GetTypeStr(),
		eventDesc,
		m_algo->GetTag(),
		GetPlanedQty(),
		GetOpenedQty(),
		GetSecurity().Descale(GetOpenStartPrice()),
		GetSecurity().Descale(GetOpenPrice()),
		GetOpenOrderId(),
		GetSecurity().GetTradeSystem().GetStringStatus(orderStatus),
		state,
		GetSecurity().GetAsk(),
		GetSecurity().GetBid());
}

void Position::ReportClosingUpdate(
			const char *eventDesc,
			TradeSystem::OrderStatus orderStatus,
			long state)
		const {
	Log::Trading(
		"position",
		"%1% %2% close-%3% %4% qty=%5%->%6% price=%7% order-id=%8%->%9%"
			" order-status=%10% state=%11% cur-ask-bid=%12%/%13%",
		GetSecurity().GetSymbol(),
		GetTypeStr(),
		eventDesc,
		m_algo->GetTag(),
		GetOpenedQty(),
		GetClosedQty(),
		GetSecurity().Descale(GetClosePrice()),
		GetOpenOrderId(),
		GetCloseOrderId(),
		GetSecurity().GetTradeSystem().GetStringStatus(orderStatus),
		state,
		GetSecurity().GetAsk(),
		GetSecurity().GetBid());
}

void Position::ReportCloseOrderChange(
			TradeSystem::OrderStatus orderStatus,
			long state,
			TradeSystem::OrderId prevOrderId,
			TradeSystem::OrderId newOrderId)
		const {
	Assert(prevOrderId != newOrderId);
	Log::Trading(
		"position",
		"%1% %2% close-order-change qty=%4%->%5% price=%6% order-id=%7%->%8%->%9%"
			" order-status=%10% state=%11% cur-ask-bid=%12%/%13%",
		GetSecurity().GetSymbol(),
		GetTypeStr(),
		m_algo->GetTag(),
		GetOpenedQty(),
		GetClosedQty(),
		GetSecurity().Descale(GetClosePrice()),
		GetOpenOrderId(),
		prevOrderId,
		newOrderId,
		GetSecurity().GetTradeSystem().GetStringStatus(orderStatus),
		state,
		GetSecurity().GetAsk(),
		GetSecurity().GetBid());
}

TradeSystem::OrderId Position::OpenAtMarketPrice() {
	const auto orderId = DoOpenAtMarketPrice();
	Interlocking::Exchange(m_opened.orderId, orderId);
	return orderId;
}

TradeSystem::OrderId Position::Open(Price price) {
	const auto orderId = DoOpen(price);
	Interlocking::Exchange(m_opened.orderId, orderId);
	return orderId;
}

TradeSystem::OrderId Position::OpenAtMarketPriceWithStopPrice(Price stopPrice) {
	const auto orderId = DoOpenAtMarketPriceWithStopPrice(stopPrice);
	Interlocking::Exchange(m_opened.orderId, orderId);
	return orderId;
}

TradeSystem::OrderId Position::OpenOrCancel(Price price) {
	const auto orderId = DoOpenOrCancel(price);
	Interlocking::Exchange(m_opened.orderId, orderId);
	return orderId;
}

TradeSystem::OrderId Position::CloseAtMarketPrice() {
	const auto orderId = DoCloseAtMarketPrice();
	Interlocking::Exchange(m_closed.orderId, orderId);
	return orderId;
}

TradeSystem::OrderId Position::Close(Price price) {
	const auto orderId = DoClose(price);
	Interlocking::Exchange(m_closed.orderId, orderId);
	return orderId;
}

TradeSystem::OrderId Position::CloseAtMarketPriceWithStopPrice(Price stopPrice) {
	const auto orderId = DoCloseAtMarketPriceWithStopPrice(stopPrice);
	Interlocking::Exchange(m_closed.orderId, orderId);
	return orderId;
}

TradeSystem::OrderId Position::CloseOrCancel(Price price) {
	const auto orderId = DoCloseOrCancel(price);
	Interlocking::Exchange(m_closed.orderId, orderId);
	return orderId;
}

// void Position::CancelPositionAtMarketPrice() {
// 	Assert(!IsClosed());
// 	Assert(GetOpenOrderId());
// 	Assert(!m_cancelMethod);
// 	m_cancelMethod = boost::bind(&PositionState::CloseAtMarketPrice, this);
// 	CancelAllOrders();
// }

void Position::CancelAllOrders() {
	Assert(!IsClosed());
	Assert(GetOpenOrderId());
	GetSecurity().CancelOrder(GetOpenOrderId());
	if (GetCloseOrderId()) {
		GetSecurity().CancelOrder(GetCloseOrderId());
	}
}

//////////////////////////////////////////////////////////////////////////

LongPosition::LongPosition(
			boost::shared_ptr<Security> security,
			Qty qty,
			Price startPrice,
			boost::shared_ptr<const Algo> algo,
			boost::shared_ptr<AlgoPositionState> state /*= boost::shared_ptr<AlgoPositionState>()*/)
		: Position(security, qty, startPrice, algo, state) {
	//...//
}

LongPosition::~LongPosition() {
	//...//
}

LongPosition::Type LongPosition::GetType() const {
	return TYPE_LONG;
}

const std::string & LongPosition::GetTypeStr() const {
	static const std::string result = "long";
	return result;
}

Security::OrderStatusUpdateSlot LongPosition::GetSellOrderStatusUpdateSlot() {
	return boost::bind(&LongPosition::UpdateClosing, shared_from_this(), _1, _2, _3, _4, _5, _6);
}

Security::OrderStatusUpdateSlot LongPosition::GetBuyOrderStatusUpdateSlot() {
	return boost::bind(&LongPosition::UpdateOpening, shared_from_this(), _1, _2, _3, _4, _5, _6);
}

TradeSystem::OrderId LongPosition::DoOpenAtMarketPrice() {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(GetNotOpenedQty() > 0);
	return GetSecurity().BuyAtMarketPrice(GetNotOpenedQty(), *this);
}

TradeSystem::OrderId LongPosition::DoOpen(Price price) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(GetNotOpenedQty() > 0);
	return GetSecurity().Buy(GetNotOpenedQty(), price, *this);
}

TradeSystem::OrderId LongPosition::DoOpenAtMarketPriceWithStopPrice(Price stopPrice) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(GetNotOpenedQty() > 0);
	return GetSecurity().BuyAtMarketPriceWithStopPrice(GetNotOpenedQty(), stopPrice, *this);
}

TradeSystem::OrderId LongPosition::DoOpenOrCancel(Price price) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(GetNotOpenedQty() > 0);
	return GetSecurity().BuyOrCancel(GetNotOpenedQty(), price, *this);
}

TradeSystem::OrderId LongPosition::DoCloseAtMarketPrice() {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(GetActiveQty() > 0);
	return GetSecurity().SellAtMarketPrice(GetActiveQty(), *this);
}

TradeSystem::OrderId LongPosition::DoClose(Price price) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(GetActiveQty() > 0);
	return GetSecurity().Sell(GetActiveQty(), price, *this);
}

TradeSystem::OrderId LongPosition::DoCloseAtMarketPriceWithStopPrice(Price stopPrice) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(GetActiveQty() > 0);
	return GetSecurity().SellAtMarketPriceWithStopPrice(GetActiveQty(), stopPrice, *this);
}

TradeSystem::OrderId LongPosition::DoCloseOrCancel(Price price) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(GetActiveQty() > 0);
	return GetSecurity().SellOrCancel(GetActiveQty(), price, *this);
}

//////////////////////////////////////////////////////////////////////////

ShortPosition::ShortPosition(
			boost::shared_ptr<Security> security,
			Qty qty,
			Price startPrice,
			boost::shared_ptr<const Algo> algo,
			boost::shared_ptr<AlgoPositionState> state /*= boost::shared_ptr<AlgoPositionState>()*/)
		: Position(security, qty, startPrice, algo, state) {
	//...//
}

ShortPosition::~ShortPosition() {
	//...//
}

LongPosition::Type ShortPosition::GetType() const {
	return TYPE_SHORT;
}

const std::string & ShortPosition::GetTypeStr() const {
	static const std::string result = "short";
	return result;
}

Security::OrderStatusUpdateSlot ShortPosition::GetSellOrderStatusUpdateSlot() {
	return boost::bind(&ShortPosition::UpdateOpening, shared_from_this(), _1, _2, _3, _4, _5, _6);
}

Security::OrderStatusUpdateSlot ShortPosition::GetBuyOrderStatusUpdateSlot() {
	return boost::bind(&ShortPosition::UpdateClosing, shared_from_this(), _1, _2, _3, _4, _5, _6);
}

TradeSystem::OrderId ShortPosition::DoOpenAtMarketPrice() {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(GetNotOpenedQty() > 0);
	return GetSecurity().SellAtMarketPrice(GetNotOpenedQty(), *this);
}

TradeSystem::OrderId ShortPosition::DoOpen(Price price) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(GetNotOpenedQty() > 0);
	return GetSecurity().Sell(GetNotOpenedQty(), price, *this);
}

TradeSystem::OrderId ShortPosition::DoOpenAtMarketPriceWithStopPrice(Price stopPrice) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(GetNotOpenedQty() > 0);
	return GetSecurity().SellAtMarketPriceWithStopPrice(GetNotOpenedQty(), stopPrice, *this);
}

TradeSystem::OrderId ShortPosition::DoOpenOrCancel(Price price) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(GetNotOpenedQty() > 0);
	return GetSecurity().SellOrCancel(GetNotOpenedQty(), price, *this);
}

TradeSystem::OrderId ShortPosition::DoCloseAtMarketPrice() {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(GetActiveQty() > 0);
	return GetSecurity().BuyAtMarketPrice(GetActiveQty(), *this);
}

TradeSystem::OrderId ShortPosition::DoClose(Price price) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(GetActiveQty() > 0);
	return GetSecurity().Buy(GetActiveQty(), price, *this);
}

TradeSystem::OrderId ShortPosition::DoCloseAtMarketPriceWithStopPrice(Price stopPrice) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(GetActiveQty() > 0);
	return GetSecurity().BuyAtMarketPriceWithStopPrice(GetActiveQty(), stopPrice, *this);
}

TradeSystem::OrderId ShortPosition::DoCloseOrCancel(Price price) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(GetActiveQty() > 0);
	return GetSecurity().BuyOrCancel(GetActiveQty(), price, *this);
}

//////////////////////////////////////////////////////////////////////////
