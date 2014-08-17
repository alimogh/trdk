/**************************************************************************
 *   Created: 2012/07/08 04:07:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Position.hpp"
#include "Strategy.hpp"
#include "Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;

//////////////////////////////////////////////////////////////////////////

namespace {
	
	const std::string logTag = "position";

	const OrderParams defaultOrderParams = {};

}

//////////////////////////////////////////////////////////////////////////

Position::LogicError::LogicError(const char *what) throw()
		: Lib::LogicError(what) {
	//...//
}
		
Position::AlreadyStartedError::AlreadyStartedError() throw()
		: LogicError("Position already started") {
	//...//
}

Position::NotOpenedError::NotOpenedError() throw()
		: LogicError("Position not opened") {
	//...//
}
		
Position::AlreadyClosedError::AlreadyClosedError() throw()
		: LogicError("Position already closed") {
	//...//
}

//////////////////////////////////////////////////////////////////////////

namespace {

	const OrderId nOrderId = std::numeric_limits<OrderId>::max();

}

class Position::Implementation : private boost::noncopyable {

public:

	//! @todo use spin read-write???
	typedef boost::shared_mutex Mutex;
	typedef boost::shared_lock<Mutex> ReadLock;
	typedef boost::unique_lock<Mutex> WriteLock;

	typedef boost::signals2::signal<StateUpdateSlotSignature>
		StateUpdateSignal;

	struct DynamicData {

		boost::atomic<OrderId> orderId;
		Time time;
		struct Price {
			boost::atomic_intmax_t total;
			boost::atomic_size_t count;
			Price()
					: total(0),
					count(0) {
				//...//
			}
		} price;
		boost::atomic<Qty> qty;
		boost::atomic<ScaledPrice> comission;

		boost::atomic_bool hasOrder;

		DynamicData()
				: orderId(nOrderId),
				qty(0),
				comission(0),
				hasOrder(false) {
			//...//
		}

	};
	
private:
	
	enum Canceled {
		CANCELED_0,
		CANCELED_1,
		CANCELED_2,
	};

public:

	Position &m_position;

	mutable Mutex m_mutex;

	mutable StateUpdateSignal m_stateUpdateSignal;

	Strategy *m_strategy;
	Security &m_security;
	const Currency m_currency;

	const Qty m_planedQty;

	const ScaledPrice m_openStartPrice;
	DynamicData m_opened;

	ScaledPrice m_closeStartPrice;
	DynamicData m_closed;

	boost::atomic<CloseType> m_closeType;

	volatile long m_isStarted;

	boost::atomic_bool m_isError;

	boost::atomic<Canceled> m_isCanceled;
	boost::function<void ()> m_cancelMethod;

	const std::string m_tag;

	explicit Implementation(
				Position &position,
				Strategy &strategy,
				Security &security,
				const Currency &currency,
				Qty qty,
				ScaledPrice startPrice)
			: m_position(position),
			m_strategy(&strategy),
			m_security(security),
			m_currency(currency),
			m_planedQty(qty),
			m_openStartPrice(startPrice),
			m_closeStartPrice(0),
			m_closeType(CLOSE_TYPE_NONE),
			m_isStarted(false),
			m_isError(false),
			m_isCanceled(CANCELED_0),
			m_tag(m_strategy->GetTag()) {
		//...//
	}

public:

	void UpdateOpening(
				OrderId orderId,
				TradeSystem::OrderStatus orderStatus,
				Qty filled,
				Qty remaining,
				double avgPrice,
				double /*lastPrice*/) {

		UseUnused(orderId);

		WriteLock lock(m_mutex);

		Assert(!m_position.IsOpened());
		Assert(!m_position.IsClosed());
		Assert(!m_position.IsCompleted());
		AssertNe(orderId, 0);
		AssertNe(m_opened.orderId, 0);
		AssertEq(m_opened.orderId, orderId);
		AssertEq(m_closed.price.total, 0);
		AssertEq(m_closed.price.count, 0);
		Assert(m_opened.time.is_not_a_date_time());
		Assert(m_closed.time.is_not_a_date_time());
		AssertLe(m_opened.qty, m_planedQty);
		AssertEq(m_closed.qty, 0);
		Assert(m_opened.hasOrder);
		Assert(!m_closed.hasOrder);

		switch (orderStatus) {
			default:
				AssertFail("Unknown order status");
				return;
			case TradeSystem::ORDER_STATUS_PENDIGN:
			case TradeSystem::ORDER_STATUS_SUBMITTED:
				AssertEq(m_opened.qty, 0);
				AssertEq(m_opened.price.total, 0);
				AssertEq(m_opened.price.count, 0);
				return;
			case TradeSystem::ORDER_STATUS_FILLED:
				Assert(filled + remaining == m_planedQty);
				Assert(m_opened.qty + filled <= m_planedQty);
				Assert(avgPrice > 0);
				m_opened.price.total += m_security.ScalePrice(avgPrice);
				++m_opened.price.count;
				m_opened.qty += filled;
				AssertGt(m_opened.qty, 0);
				ReportOpeningUpdate("filled", orderStatus);
				if (remaining != 0) {
					return;
				}
				break;
			case TradeSystem::ORDER_STATUS_INACTIVE:
			case TradeSystem::ORDER_STATUS_ERROR:
				ReportOpeningUpdate("error", orderStatus);
				m_isError = true;
				break;
			case TradeSystem::ORDER_STATUS_CANCELLED:
				ReportOpeningUpdate("canceled", orderStatus);
				break;
		}
	
		AssertLe(m_opened.qty, m_planedQty);
		Assert(m_opened.time.is_not_a_date_time());

		if (m_position.GetOpenedQty() > 0) {
			try {
				m_opened.time
					= !m_security.GetContext().GetSettings().IsReplayMode()
						?	boost::get_system_time()
						:	m_security.GetLastMarketDataTime();
			} catch (...) {
				AssertFailNoException();
			}
		}

		m_opened.hasOrder = false;

		if (CancelIfSet()) {
			return;
		}

		lock.unlock();
		SignalUpdate();

	}

	void UpdateClosing(
				OrderId orderId,
				TradeSystem::OrderStatus orderStatus,
				Qty filled,
				Qty remaining,
				double avgPrice,
				double /*lastPrice*/) {

		UseUnused(orderId);

		WriteLock lock(m_mutex);

		Assert(m_position.IsOpened());
		Assert(!m_position.IsClosed());
		Assert(!m_position.IsCompleted());
		Assert(!m_opened.time.is_not_a_date_time());
		Assert(m_closed.time.is_not_a_date_time());
		AssertLe(m_opened.qty, m_planedQty);
		AssertLe(m_closed.qty, m_opened.qty);
		AssertNe(orderId, 0);
		AssertNe(m_closed.orderId, 0);
		AssertEq(m_closed.orderId, orderId);
		AssertNe(m_opened.orderId, orderId);
		Assert(!m_opened.hasOrder);
		Assert(m_closed.hasOrder);

		switch (orderStatus) {
			default:
				AssertFail("Unknown order status");
				return;
			case TradeSystem::ORDER_STATUS_PENDIGN:
			case TradeSystem::ORDER_STATUS_SUBMITTED:
				AssertEq(m_closed.qty, 0);
				AssertEq(m_closed.price.total, 0);
				AssertEq(m_closed.price.count, 0);
				return;
			case TradeSystem::ORDER_STATUS_FILLED:
				AssertEq(filled + remaining, m_opened.qty);
				AssertLe(long(m_closed.qty) + filled, m_opened.qty);
				Assert(avgPrice > 0);
				m_closed.price.total += m_security.ScalePrice(avgPrice);
				++m_closed.price.count;
				m_closed.qty += filled;
				AssertGt(m_closed.qty, 0);
				ReportClosingUpdate("filled", orderStatus);
				if (remaining != 0) {
					return;
				}
				break;
			case TradeSystem::ORDER_STATUS_INACTIVE:
			case TradeSystem::ORDER_STATUS_ERROR:
//! @todo Log THIS!!!
// 				m_security.GetContext().GetLog().Error(
// 					"Position CLOSE error: symbol: \"%1%\", strategy %2%"
// 						", trade system state: %3%, orders ID: %4%->%5%, qty: %6%->%7%.",
// 					boost::make_tuple(
// 						boost::cref(m_position.GetSecurity()),
// 						boost::cref(m_tag),
// 						orderStatus,
// 						m_opened.orderId,
// 						m_closed.orderId,	
// 						m_opened.qty,
// 						m_closed.qty));
				ReportClosingUpdate("error", orderStatus);
				m_isError = true;
				break;
			case TradeSystem::ORDER_STATUS_CANCELLED:
				ReportClosingUpdate("canceled", orderStatus);
				break;
		}
	
		AssertLe(m_opened.qty, m_planedQty);
		Assert(m_closed.time.is_not_a_date_time());

		if (m_position.GetActiveQty() == 0) {
			try {
				m_closed.time
					= !m_security.GetContext().GetSettings().IsReplayMode()
						?	boost::get_system_time()
						:	m_security.GetLastMarketDataTime();
			} catch (...) {
				m_isError = true;
				AssertFailNoException();
			}
		}

		m_closed.hasOrder = false;

		if (CancelIfSet()) {
			return;
		}

		lock.unlock();
		SignalUpdate();		

	}

	void SignalUpdate() {
		m_stateUpdateSignal();
	}

public:

	bool CancelIfSet() throw() {
		if (m_position.IsClosed()) {
			return false;
		}
		Canceled prevCancelFlag = CANCELED_1;
		const Canceled newCancelFlag = CANCELED_2;
		if  (!m_isCanceled.compare_exchange_strong(prevCancelFlag, newCancelFlag)) {
			return false;
		}
//! @todo Log THIS!!!
// 		m_security.GetContext().GetLog().Trading(
// 			logTag,
// 			"%1% %2% close-cancel-post %3% qty=%4%->%5% price=market order-id=%6%->%7%"
// 				" has-orders=%8%/%9% is-error=%10%",
// 			boost::make_tuple(
// 				boost::cref(m_security.GetSymbol()),
// 				boost::cref(m_position.GetTypeStr()),
// 				boost::cref(m_tag),
// 				m_position.GetOpenedQty(),
// 				m_position.GetClosedQty(),
// 				m_position.GetOpenOrderId(),
// 				m_position.GetCloseOrderId(),
// 				m_position.HasActiveOpenOrders(),
// 				m_position.HasActiveCloseOrders(),
// 				m_isError ? true : false));
		try {
			m_cancelMethod();
			return true;
		} catch (...) {
			m_isError = true;
			AssertFailNoException();
			return false;
		}
	}

	void ReportOpeningUpdate(
				const char *eventDesc,
				TradeSystem::OrderStatus orderStatus)
			const
			throw() {
		try {
			const auto &context = m_security.GetContext();
			context.GetLog().Trading(
				logTag,
				"%1% %2% open-%3% %4% qty=%5%->%6% price=%7%->%8% order-id=%9%"
					" order-status=%10%",
				boost::make_tuple(
					boost::cref(m_security.GetSymbol()),
					boost::cref(m_position.GetTypeStr()),
					eventDesc,
					boost::cref(m_tag),
					m_position.GetPlanedQty(),
					m_position.GetOpenedQty(),
					m_security.DescalePrice(m_position.GetOpenStartPrice()),
					m_security.DescalePrice(m_position.GetOpenPrice()),
					m_position.GetOpenOrderId(),
					context.GetTradeSystem().GetStringStatus(orderStatus)));
		} catch (...) {
			AssertFailNoException();
		}
	}
	
	void ReportClosingUpdate(
				const char *eventDesc,
				TradeSystem::OrderStatus orderStatus)
			const
			throw() {
		try {
			const auto &context = m_security.GetContext();
			context.GetLog().Trading(
				logTag,
				"%1% %2% close-%3% %4% qty=%5%->%6% price=%7% order-id=%8%->%9%"
					" order-status=%10%",
				boost::make_tuple(
					boost::cref(m_position.GetSecurity().GetSymbol()),
					boost::cref(m_position.GetTypeStr()),
					eventDesc,
					boost::cref(m_tag),
					m_position.GetOpenedQty(),
					m_position.GetClosedQty(),
					m_position
						.GetSecurity()
						.DescalePrice(m_position.GetClosePrice()),
					m_position.GetOpenOrderId(),
					m_position.GetCloseOrderId(),
					context.GetTradeSystem().GetStringStatus(orderStatus)));
		} catch (...) {
			AssertFailNoException();
		}
	}

public:

	void RestoreOpenState(OrderId openOrderId) {
		
		const WriteLock lock(m_mutex);
		
		Assert(!m_position.IsOpened());
		Assert(!m_position.IsError());
		Assert(!m_position.IsClosed());
		Assert(!m_position.IsCompleted());
		Assert(!m_position.HasActiveOrders());
		AssertLt(0, m_planedQty);
		AssertEq(0, m_opened.qty);
		AssertEq(0, m_closed.qty);
		Assert(m_opened.time.is_not_a_date_time());
		Assert(m_closed.time.is_not_a_date_time());

		if (m_position.IsStarted()) {
			throw AlreadyStartedError();
		}

		m_opened.time = boost::get_system_time();
		m_opened.qty = m_planedQty;
		m_opened.orderId = openOrderId;

		if (m_strategy) {
			m_strategy->Register(m_position);
			m_strategy = nullptr;
		}

		ReportOpeningUpdate("restored", TradeSystem::ORDER_STATUS_FILLED);

		SignalUpdate();
	
	}

	template<typename OpenImpl>
	OrderId Open(const OpenImpl &openImpl) {
		
		const WriteLock lock(m_mutex);
		if (m_position.IsStarted()) {
			throw AlreadyStartedError();
		}
		
		Assert(!m_position.IsOpened());
		Assert(!m_position.IsError());
		Assert(!m_position.IsClosed());
		Assert(!m_position.IsCompleted());
		Assert(!m_position.HasActiveOrders());

		if (m_strategy) {
			m_strategy->Register(m_position);
		}
		try {
			const auto orderId = openImpl(m_position.GetNotOpenedQty());
			m_strategy = nullptr;
			m_opened.hasOrder = true;
			AssertEq(nOrderId, m_opened.orderId);
			m_opened.orderId = orderId;
			Assert(!m_isStarted);
			m_isStarted = true;
			return orderId;
		} catch (...) {
			if (m_strategy) {
				m_strategy->Unregister(m_position);
			}
			throw;
		}
	
	}

	template<typename CloseImpl>
	OrderId CloseUnsafe(CloseType closeType, const CloseImpl &closeImpl) {
		
		if (!m_position.IsOpened()) {
			throw NotOpenedError();
		} else if (m_position.HasActiveCloseOrders() || m_position.IsClosed()) {
			throw AlreadyClosedError();
		}

		Assert(!m_strategy);
		Assert(m_position.IsStarted());
		Assert(!m_position.IsError());
		Assert(!m_position.HasActiveOrders());
		Assert(!m_position.IsCompleted());

		const auto orderId = closeImpl(m_position.GetActiveQty());
		m_closeType = closeType;
		m_closed.hasOrder = true;
		m_closed.orderId = orderId;

		return orderId;

	}

	template<typename CloseImpl>
	OrderId Close(CloseType closeType, const CloseImpl &closeImpl) {
		const Implementation::WriteLock lock(m_mutex);
		return CloseUnsafe(closeType, closeImpl);
	}

	template<typename CancelMethodImpl>
	bool CancelAtMarketPrice(
				CloseType closeType,
				const CancelMethodImpl &cancelMethodImpl) {
		const WriteLock lock(m_mutex);
		if (m_position.IsCanceled() || m_position.GetActiveQty() == 0) {
			return false;
		}
		m_security.GetContext().GetLog().Trading(
			logTag,
			"%1% %2% close-cancel-pre %3% qty=%4%->%5%"
				" price=market order-id=%6%->%7%"
				" has-orders=%8%/%9% is-error=%10%",
			boost::make_tuple(
				boost::cref(m_security),
				boost::cref(m_position.GetTypeStr()),
				boost::cref(m_tag),
				m_position.GetOpenedQty(),
				m_position.GetClosedQty(),
				m_position.GetOpenOrderId(),
				m_position.GetCloseOrderId(),
				m_position.HasActiveOpenOrders(),
				m_position.HasActiveCloseOrders(),
				m_isError ? true : false));
		if (	m_position.IsClosed()
				|| (	!m_position.IsOpened()
						&& !m_position.HasActiveOpenOrders())) {
			return false;
		}
		boost::function<void ()> cancelMethod = boost::bind(
			&Implementation::CloseUnsafe<CancelMethodImpl>,
			this,
			closeType,
			cancelMethodImpl);
		if (CancelAllOrders()) {
			Assert(m_position.HasActiveOrders());
			cancelMethod.swap(m_cancelMethod);
		} else {
			Assert(!m_position.HasActiveOrders());
			cancelMethod();
		}
		AssertEq(int(CANCELED_0), int(m_isCanceled));
		m_isCanceled = CANCELED_1;
		return true;
	}

	bool CancelAllOrders() {
		bool isCanceled = false;
		if (m_opened.hasOrder) {
			m_security.CancelOrder(m_opened.orderId);
			isCanceled = true;
		}
		if (m_closed.hasOrder) {
			Assert(!isCanceled);
			m_security.CancelOrder(m_closed.orderId);
			isCanceled = true;
		}
		return isCanceled;
	}

};


//////////////////////////////////////////////////////////////////////////

Position::Position(
			Strategy &strategy,
			Security &security,
			const Currency &currency,
			Qty qty,
			ScaledPrice startPrice) {
	m_pimpl = new Implementation(
		*this,
		strategy,
		security,
		currency,
		qty,
		startPrice);
	AssertGt(m_pimpl->m_planedQty, 0);
}

Position::~Position() {
	delete m_pimpl;
}

const Security & Position::GetSecurity() const throw() {
	return const_cast<Position *>(this)->GetSecurity();
}
Security & Position::GetSecurity() throw() {
	return m_pimpl->m_security;
}

const Currency & Position::GetCurrency() const throw() {
	return m_pimpl->m_currency;
}

Position::CloseType Position::GetCloseType() const throw() {
	return CloseType(m_pimpl->m_closeType);
}

bool Position::IsOpened() const throw() {
	return !HasActiveOpenOrders() && GetOpenedQty() > 0;
}
bool Position::IsClosed() const throw() {
	return
		!HasActiveOrders()
		&& GetOpenedQty() > 0
		&& GetActiveQty() == 0;
}

bool Position::IsStarted() const throw() {
	return m_pimpl->m_opened.orderId != nOrderId;
}

bool Position::IsCompleted() const throw() {
	return IsStarted() && !HasActiveOrders() && GetActiveQty() == 0;
}

bool Position::IsError() const throw() {
	return m_pimpl->m_isError ? true : false;
}
bool Position::IsCanceled() const throw() {
	return m_pimpl->m_isCanceled > 0;
}

bool Position::HasActiveOrders() const throw() {
	return HasActiveCloseOrders() || HasActiveOpenOrders();
}
bool Position::HasActiveOpenOrders() const throw() {
	return m_pimpl->m_opened.hasOrder ? true : false;
}
bool Position::HasActiveCloseOrders() const throw() {
	return m_pimpl->m_closed.hasOrder ? true : false;
}

namespace { namespace CloseTypeStr {
	const std::string none = "-";
	const std::string takeProfit = "t/p";
	const std::string stopLoss = "s/l";
	const std::string timeout = "timeout";
	const std::string schedule = "schedule";
	const std::string engineStop = "engine stop";
} }

const std::string & Position::GetCloseTypeStr() const {
	using namespace CloseTypeStr;
	switch (GetCloseType()) {
		default:
			AssertFail("Unknown position close type.");
		case CLOSE_TYPE_NONE:
			return none;
		case CLOSE_TYPE_TAKE_PROFIT:
			return takeProfit;
		case CLOSE_TYPE_STOP_LOSS:
			return stopLoss;
		case CLOSE_TYPE_TIMEOUT:
			return timeout;
		case CLOSE_TYPE_SCHEDULE:
			return schedule;
		case CLOSE_TYPE_ENGINE_STOP:
			return engineStop;
	}
}

void Position::UpdateOpening(
			OrderId orderId,
			TradeSystem::OrderStatus orderStatus,
			Qty filled,
			Qty remaining,
			double avgPrice,
			double lastPrice) {
	m_pimpl->UpdateOpening(
		orderId,
		orderStatus,
		filled,
		remaining,
		avgPrice,
		lastPrice);
}

void Position::UpdateClosing(
			OrderId orderId,
			TradeSystem::OrderStatus orderStatus,
			Qty filled,
			Qty remaining,
			double avgPrice,
			double lastPrice) {
	m_pimpl->UpdateClosing(
		orderId,
		orderStatus,
		filled,
		remaining,
		avgPrice,
		lastPrice);
}

Position::Time Position::GetOpenTime() const {
	Time result;
	{
		const Implementation::ReadLock lock(m_pimpl->m_mutex);
		result = m_pimpl->m_opened.time;
	}
	return result;
}

Qty Position::GetNotOpenedQty() const {
	AssertLe(GetOpenedQty(), GetPlanedQty());
	return GetPlanedQty() - GetOpenedQty();
}

Qty Position::GetActiveQty() const throw() {
	AssertGe(GetOpenedQty(), GetClosedQty());
	return GetOpenedQty() - GetClosedQty();
}

OrderId Position::GetCloseOrderId() const throw() {
	return m_pimpl->m_closed.orderId;
}

Qty Position::GetClosedQty() const throw() {
	return m_pimpl->m_closed.qty;
}

Position::Time Position::GetCloseTime() const {
	Position::Time result;
	{
		const Implementation::ReadLock lock(m_pimpl->m_mutex);
		result = m_pimpl->m_closed.time;
	}
	return result;
}

ScaledPrice Position::GetCommission() const {
	return m_pimpl->m_opened.comission + m_pimpl->m_closed.comission;
}

Position::StateUpdateConnection Position::Subscribe(
			const StateUpdateSlot &slot)
		const {
	return StateUpdateConnection(m_pimpl->m_stateUpdateSignal.connect(slot));
}

Qty Position::GetPlanedQty() const {
	return m_pimpl->m_planedQty;
}

ScaledPrice Position::GetOpenStartPrice() const {
	return m_pimpl->m_openStartPrice;
}

OrderId Position::GetOpenOrderId() const throw() {
	return m_pimpl->m_opened.orderId;
}
Qty Position::GetOpenedQty() const throw() {
	return m_pimpl->m_opened.qty;
}

ScaledPrice Position::GetOpenPrice() const {
	return m_pimpl->m_opened.price.count > 0
		?	m_pimpl->m_opened.price.total / m_pimpl->m_opened.price.count
		:	0;
}

ScaledPrice Position::GetCloseStartPrice() const {
	return m_pimpl->m_closeStartPrice;
}

void Position::SetCloseStartPrice(ScaledPrice price) {
	m_pimpl->m_closeStartPrice = price;
}

ScaledPrice Position::GetClosePrice() const {
	return m_pimpl->m_closed.price.count > 0
		?	m_pimpl->m_closed.price.total / m_pimpl->m_closed.price.count
		:	0;
}

OrderId Position::OpenAtMarketPrice() {
	return OpenAtMarketPrice(defaultOrderParams);
}

OrderId Position::OpenAtMarketPrice(const OrderParams &params) {
	return m_pimpl->Open(
		[&](Qty qty) -> OrderId {
			return DoOpenAtMarketPrice(qty, params);
		});
}

OrderId Position::Open(ScaledPrice price) {
	return Open(price, defaultOrderParams);
}

OrderId Position::Open(ScaledPrice price, const OrderParams &params) {
	return m_pimpl->Open(
		[&](Qty qty) -> OrderId {
			return DoOpen(qty, price, params);
		});
}

OrderId Position::OpenAtMarketPriceWithStopPrice(ScaledPrice stopPrice) {
	return OpenAtMarketPriceWithStopPrice(stopPrice, defaultOrderParams);
}

OrderId Position::OpenAtMarketPriceWithStopPrice(
			ScaledPrice stopPrice,
			const OrderParams &params) {
	return m_pimpl->Open(
		[&](Qty qty) -> OrderId {
			return DoOpenAtMarketPriceWithStopPrice(qty, stopPrice, params);
		});
}

OrderId Position::OpenOrCancel(ScaledPrice price) {
	return OpenOrCancel(price, defaultOrderParams);
}

OrderId Position::OpenOrCancel(ScaledPrice price, const OrderParams &params) {
	return m_pimpl->Open(
		[&](Qty qty) -> OrderId {
			return DoOpenOrCancel(qty, price, params);
		});
}

OrderId Position::CloseAtMarketPrice(CloseType closeType) {
	return CloseAtMarketPrice(closeType, defaultOrderParams);
}

OrderId Position::CloseAtMarketPrice(
			CloseType closeType,
			const OrderParams &params) {
	return m_pimpl->Close(
		closeType,
		[&](Qty qty) -> OrderId {
			return DoCloseAtMarketPrice(qty, params);
		});
}

OrderId Position::Close(CloseType closeType, ScaledPrice price) {
	return Close(closeType, price, defaultOrderParams);
}

OrderId Position::Close(
			CloseType closeType,
			ScaledPrice price,
			const OrderParams &params) {
	return m_pimpl->Close(
		closeType,
		[&](Qty qty) -> OrderId {
			return DoClose(qty, price, params);
		});
}

OrderId Position::CloseAtMarketPriceWithStopPrice(
			CloseType closeType,
			ScaledPrice stopPrice) {
	return CloseAtMarketPriceWithStopPrice(
		closeType,
		stopPrice,
		defaultOrderParams);
}

OrderId Position::CloseAtMarketPriceWithStopPrice(
			CloseType closeType,
			ScaledPrice stopPrice,
			const OrderParams &params) {
	return m_pimpl->Close(
		closeType,
		[&](Qty qty) -> OrderId {
			return DoCloseAtMarketPriceWithStopPrice(qty, stopPrice, params);
		});
}

OrderId Position::CloseOrCancel(CloseType closeType, ScaledPrice price) {
	return CloseOrCancel(closeType, price, defaultOrderParams);
}

OrderId Position::CloseOrCancel(
			CloseType closeType,
			ScaledPrice price,
			const OrderParams &params) {
	return m_pimpl->Close(
		closeType,
		[&](Qty qty) -> OrderId {
			return DoCloseOrCancel(qty, price, params);
		});
}

bool Position::CancelAtMarketPrice(CloseType closeType) {
	return CancelAtMarketPrice(closeType, defaultOrderParams);
}

bool Position::CancelAtMarketPrice(
			CloseType closeType,
			const OrderParams &params) {
	return m_pimpl->CancelAtMarketPrice(
		closeType,
		[&](Qty qty) -> OrderId {
			return DoCloseAtMarketPrice(qty, params);
		});
}

bool Position::CancelAllOrders() {
	const Implementation::WriteLock lock(m_pimpl->m_mutex);
	return m_pimpl->CancelAllOrders();
}

void Position::RestoreOpenState(OrderId openOrderId) {
	m_pimpl->RestoreOpenState(openOrderId);
}

//////////////////////////////////////////////////////////////////////////

LongPosition::LongPosition(
			Strategy &strategy,
			Security &security,
			const Currency &currency,
			Qty qty,
			ScaledPrice startPrice)
		: Position(strategy, security, currency, qty, startPrice) {
	//...//
}

LongPosition::~LongPosition() {
	//...//
}

LongPosition::Type LongPosition::GetType() const {
	return TYPE_LONG;
}

namespace {
	const std::string longPositionTypeName = "long";
}
const std::string & LongPosition::GetTypeStr() const throw() {
	return longPositionTypeName;
}

Security::OrderStatusUpdateSlot LongPosition::GetSellOrderStatusUpdateSlot() {
	return boost::bind(
		&LongPosition::UpdateClosing,
		shared_from_this(),
		_1,
		_2,
		_3,
		_4,
		_5,
		_6);
}

Security::OrderStatusUpdateSlot LongPosition::GetBuyOrderStatusUpdateSlot() {
	return boost::bind(
		&LongPosition::UpdateOpening,
		shared_from_this(),
		_1,
		_2,
		_3,
		_4,
		_5,
		_6);
}

OrderId LongPosition::DoOpenAtMarketPrice(Qty qty, const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetSecurity().BuyAtMarketPrice(qty, GetCurrency(), params, *this);
}

OrderId LongPosition::DoOpen(Qty qty, ScaledPrice price, const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetSecurity().Buy(qty, GetCurrency(), price, params, *this);
}

OrderId LongPosition::DoOpenAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice,
			const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetSecurity().BuyAtMarketPriceWithStopPrice(
		qty,
		GetCurrency(),
		stopPrice,
		params,
		*this);
}

OrderId LongPosition::DoOpenOrCancel(
			Qty qty,
			ScaledPrice price,
			const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetSecurity().BuyOrCancel(qty, GetCurrency(), price, params, *this);
}

OrderId LongPosition::DoCloseAtMarketPrice(Qty qty, const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetSecurity().SellAtMarketPrice(qty, GetCurrency(), params, *this);
}

OrderId LongPosition::DoClose(Qty qty, ScaledPrice price, const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetSecurity().Sell(qty, GetCurrency(), price, params, *this);
}

OrderId LongPosition::DoCloseAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice,
			const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetSecurity().SellAtMarketPriceWithStopPrice(
		qty,
		GetCurrency(), 
		stopPrice,
		params,
		*this);
}

OrderId LongPosition::DoCloseOrCancel(
			Qty qty,
			ScaledPrice price,
			const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().SellOrCancel(qty, GetCurrency(), price, params, *this);
}

//////////////////////////////////////////////////////////////////////////

ShortPosition::ShortPosition(
			Strategy &strategy,
			Security &security,
			const Currency &currency,
			Qty qty,
			ScaledPrice startPrice)
		: Position(strategy, security, currency, abs(qty), startPrice) {
	//...//
}

ShortPosition::~ShortPosition() {
	//...//
}

LongPosition::Type ShortPosition::GetType() const {
	return TYPE_SHORT;
}

namespace {
	const std::string shortTypeName = "short";
}
const std::string & ShortPosition::GetTypeStr() const throw() {
	return shortTypeName;
}

Security::OrderStatusUpdateSlot ShortPosition::GetSellOrderStatusUpdateSlot() {
	return boost::bind(
		&ShortPosition::UpdateOpening,
		shared_from_this(),
		_1,
		_2,
		_3,
		_4,
		_5,
		_6);
}

Security::OrderStatusUpdateSlot ShortPosition::GetBuyOrderStatusUpdateSlot() {
	return boost::bind(
		&ShortPosition::UpdateClosing,
		shared_from_this(),
		_1,
		_2,
		_3,
		_4,
		_5,
		_6);
}

OrderId ShortPosition::DoOpenAtMarketPrice(Qty qty, const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetSecurity().SellAtMarketPrice(qty, GetCurrency(), params, *this);
}

OrderId ShortPosition::DoOpen(Qty qty, ScaledPrice price, const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetSecurity().Sell(qty, GetCurrency(), price, params, *this);
}

OrderId ShortPosition::DoOpenAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice,
			const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetSecurity().SellAtMarketPriceWithStopPrice(
		qty,
		GetCurrency(), 
		stopPrice,
		params,
		*this);
}

OrderId ShortPosition::DoOpenOrCancel(
			Qty qty,
			ScaledPrice price,
			const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetSecurity().SellOrCancel(qty, GetCurrency(), price, params, *this);
}

OrderId ShortPosition::DoCloseAtMarketPrice(
			Qty qty,
			const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetSecurity().BuyAtMarketPrice(qty, GetCurrency(), params, *this);
}

OrderId ShortPosition::DoClose(
			Qty qty,
			ScaledPrice price,
			const OrderParams &params) {
	return GetSecurity().Buy(qty, GetCurrency(), price, params, *this);
}

OrderId ShortPosition::DoCloseAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice,
			const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetSecurity().BuyAtMarketPriceWithStopPrice(
		qty,
		GetCurrency(), 
		stopPrice,
		params,
		*this);
}

OrderId ShortPosition::DoCloseOrCancel(
			Qty qty,
			ScaledPrice price,
			const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetSecurity().BuyOrCancel(qty, GetCurrency(), price, params, *this);
}

//////////////////////////////////////////////////////////////////////////
