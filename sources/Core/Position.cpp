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

	template<Lib::Concurrency::Profile profile>
	struct PostionConcurrencyPolicyT {
		static_assert(
			profile == Lib::Concurrency::PROFILE_RELAX,
			"Wrong concurrency profile");
		typedef boost::shared_mutex Mutex;
		typedef boost::shared_lock<Mutex> ReadLock;
		typedef boost::unique_lock<Mutex> WriteLock;
	};
	template<>
	struct PostionConcurrencyPolicyT<Lib::Concurrency::PROFILE_HFT> {
		//! @todo TRDK-167
		typedef Lib::Concurrency::SpinMutex Mutex;
		typedef Mutex::ScopedLock ReadLock;
		typedef Mutex::ScopedLock WriteLock;
	};
	typedef PostionConcurrencyPolicyT<TRDK_CONCURRENCY_PROFILE>
		PostionConcurrencyPolicy;
	
}

class Position::Implementation : private boost::noncopyable {

public:


	typedef PostionConcurrencyPolicy::Mutex Mutex;
	typedef PostionConcurrencyPolicy::ReadLock ReadLock;
	typedef PostionConcurrencyPolicy::WriteLock WriteLock;

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
	
	enum CancelState {
		CANCEL_STATE_NOT_CANCELED,
		CANCEL_STATE_SCHEDULED,
		CANCEL_STATE_CANCELED,
	};

public:

	Position &m_position;

	TradeSystem &m_tradeSystem;

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

	boost::atomic<CancelState> m_cancelState;
	boost::function<void ()> m_cancelMethod;

	const std::string m_tag;

	TimeMeasurement::Milestones m_timeMeasurement;

	explicit Implementation(
				Position &position,
				TradeSystem &tradeSystem,
				Strategy &strategy,
				Security &security,
				const Currency &currency,
				Qty qty,
				ScaledPrice startPrice,
				const TimeMeasurement::Milestones &timeMeasurement)
			: m_position(position),
			m_tradeSystem(tradeSystem),
			m_strategy(&strategy),
			m_security(security),
			m_currency(currency),
			m_planedQty(qty),
			m_openStartPrice(startPrice),
			m_closeStartPrice(0),
			m_closeType(CLOSE_TYPE_NONE),
			m_isStarted(false),
			m_isError(false),
			m_cancelState(CANCEL_STATE_NOT_CANCELED),
			m_tag(m_strategy->GetTag()),
			m_timeMeasurement(timeMeasurement) {
		//...//
	}

public:

	void UpdateOpening(
				OrderId orderId,
				TradeSystem::OrderStatus orderStatus,
				Qty filled,
				Qty remaining,
				double avgPrice) {

		UseUnused(orderId);

		{

			const WriteLock lock(m_mutex);

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
					AssertEq(m_planedQty, remaining);
					AssertEq(0, filled);
					AssertEq(0, avgPrice);
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

		}
		
		SignalUpdate();

	}

	void UpdateClosing(
				OrderId orderId,
				TradeSystem::OrderStatus orderStatus,
				Qty filled,
				Qty remaining,
				double avgPrice) {

		UseUnused(orderId);

		{

			const WriteLock lock(m_mutex);

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

		}
		
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
		CancelState prevCancelFlag = CANCEL_STATE_SCHEDULED;
		const CancelState newCancelFlag = CANCEL_STATE_CANCELED;
		if  (
				!m_cancelState.compare_exchange_strong(
					prevCancelFlag,
					newCancelFlag)) {
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
		m_security.GetContext().GetLog().TradingEx(
			logTag,
			[&]() -> boost::format {
				boost::format message(
					"%11%\t%1%\t%2%\topen-%3%\t%4%"
						"\tqty=%5%->%6% price=%7%->%8% order-id=%9%"
						" order-status=%10%");
				message
					%	m_security.GetSymbol()
					%	m_position.GetTypeStr()
					%	eventDesc
					%	m_tag
					%	m_position.GetPlanedQty()
					%	m_position.GetOpenedQty()
					%	m_security.DescalePrice(m_position.GetOpenStartPrice())
					%	m_security.DescalePrice(m_position.GetOpenPrice())
					%	m_position.GetOpenOrderId()
					%	m_tradeSystem.GetStringStatus(orderStatus)
					%	m_position.GetTradeSystem().GetTag();
				return std::move(message);
			});
	}
	
	void ReportClosingUpdate(
				const char *eventDesc,
				TradeSystem::OrderStatus orderStatus)
			const
			throw() {
		m_security.GetContext().GetLog().TradingEx(
			logTag,
			[&]() -> boost::format {
				boost::format message(
					"%11%\t%1%\t%2%\tclose-%3%\t%4%"
						"\tqty=%5%->%6% price=%7% order-id=%8%->%9%"
						" order-status=%10%");
				message
					%	m_position.GetSecurity().GetSymbol()
					%	m_position.GetTypeStr()
					%	eventDesc
					%	m_tag
					%	m_position.GetOpenedQty()
					%	m_position.GetClosedQty()
					%	m_position
							.GetSecurity()
							.DescalePrice(m_position.GetClosePrice())
					%	m_position.GetOpenOrderId()
					%	m_position.GetCloseOrderId()
					%	m_tradeSystem.GetStringStatus(orderStatus)
					%	m_position.GetTradeSystem().GetTag();
				return std::move(message);
			});
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

		m_security.GetContext().GetLog().TradingEx(
			logTag,
			[&]() -> boost::format {
				boost::format message("%5%\t%1%\t%2%\topen-pre\t%3%\tqty=%4%");
				message
					%	m_security
					%	m_position.GetTypeStr()
					%	m_tag
					%	m_position.GetPlanedQty()
					%	m_position.GetTradeSystem().GetTag();
				return std::move(message);
			});

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
		if (m_position.IsCanceled()) {
			Assert(!m_position.HasActiveOpenOrders());
			Assert(!m_position.HasActiveCloseOrders());
			return false;
		}
		m_security.GetContext().GetLog().TradingEx(
			logTag,
			[&]() -> boost::format {
				boost::format message(
					"%11% %1% %2% close-cancel-pre %3% qty=%4%->%5%"
						" price=market order-id=%6%->%7%"
						" has-orders=%8%/%9% is-error=%10%");
				message
					%	m_security
					%	m_position.GetTypeStr()
					%	m_tag
					%	m_position.GetOpenedQty()
					%	m_position.GetClosedQty()
					%	m_position.GetOpenOrderId()
					%	m_position.GetCloseOrderId()
					%	m_position.HasActiveOpenOrders()
					%	m_position.HasActiveCloseOrders()
					%	(m_isError ? true : false)
					%	m_position.GetTradeSystem().GetTag();
				return std::move(message);
			});
		if (	m_position.IsClosed()
				|| (	!m_position.IsOpened()
						&& !m_position.HasActiveOpenOrders())) {
			return false;
		}
		if (CancelAllOrders()) {
			boost::function<void ()> delayedCancelMethod
				= [this, closeType, cancelMethodImpl]() {
					if (!m_position.IsOpened() || m_position.IsClosed()) {
						SignalUpdate();
						return;
					}
					CloseUnsafe<CancelMethodImpl>(closeType, cancelMethodImpl);
				};
			Assert(m_position.HasActiveOrders());
			delayedCancelMethod.swap(m_cancelMethod);
			AssertEq(int(CANCEL_STATE_NOT_CANCELED), int(m_cancelState));
			m_cancelState = CANCEL_STATE_SCHEDULED;
		} else {
			Assert(!m_position.HasActiveOrders());
			CloseUnsafe<CancelMethodImpl>(closeType, cancelMethodImpl);
			AssertEq(int(CANCEL_STATE_NOT_CANCELED), int(m_cancelState));
			m_cancelState = CANCEL_STATE_CANCELED;
		}
		return true;
	}

	bool CancelAllOrders() {
		bool isCanceled = false;
		if (m_opened.hasOrder) {
			m_tradeSystem.CancelOrder(m_opened.orderId);
			isCanceled = true;
		}
		if (m_closed.hasOrder) {
			Assert(!isCanceled);
			m_tradeSystem.CancelOrder(m_closed.orderId);
			isCanceled = true;
		}
		return isCanceled;
	}

};


//////////////////////////////////////////////////////////////////////////

Position::Position(
			Strategy &strategy,
			TradeSystem &tradeSystem,
			Security &security,
			const Currency &currency,
			Qty qty,
			ScaledPrice startPrice,
			const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl = new Implementation(
		*this,
		tradeSystem,
		strategy,
		security,
		currency,
		qty,
		startPrice,
		timeMeasurement);
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

const TradeSystem & Position::GetTradeSystem() const {
	return const_cast<Position *>(this)->GetTradeSystem();
}

TradeSystem & Position::GetTradeSystem() {
	return m_pimpl->m_tradeSystem;
}

const Currency & Position::GetCurrency() const {
	return m_pimpl->m_currency;
}

TimeMeasurement::Milestones & Position::GetTimeMeasurement() {
	return m_pimpl->m_timeMeasurement;
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
	return m_pimpl->m_cancelState != Implementation::CANCEL_STATE_NOT_CANCELED;
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
	const std::string openFailed = "open failed";
	const std::string systemError = "sys error";
} }

const std::string & Position::GetCloseTypeStr() const {
	using namespace CloseTypeStr;
	static_assert(numberOfCloseTypes == 8, "Close type list changed.");
	switch (GetCloseType()) {
		default:
			AssertEq(CLOSE_TYPE_NONE, GetCloseType());
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
		case CLOSE_TYPE_OPEN_FAILED:
			return openFailed;
		case CLOSE_TYPE_SYSTEM_ERROR:
			return systemError;
	}
}

void Position::UpdateOpening(
			OrderId orderId,
			TradeSystem::OrderStatus orderStatus,
			Qty filled,
			Qty remaining,
			double avgPrice) {
	m_pimpl->UpdateOpening(
		orderId,
		orderStatus,
		filled,
		remaining,
		avgPrice);
}

void Position::UpdateClosing(
			OrderId orderId,
			TradeSystem::OrderStatus orderStatus,
			Qty filled,
			Qty remaining,
			double avgPrice) {
	m_pimpl->UpdateClosing(
		orderId,
		orderStatus,
		filled,
		remaining,
		avgPrice);
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
void Position::SetOpenedQty(const Qty &newQty) const throw() {
	const Implementation::WriteLock lock(m_pimpl->m_mutex);
	m_pimpl->m_opened.qty = newQty;
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

OrderId Position::OpenImmediatelyOrCancel(const ScaledPrice &price) {
	return OpenImmediatelyOrCancel(price, defaultOrderParams);
}

OrderId Position::OpenImmediatelyOrCancel(ScaledPrice price, const OrderParams &params) {
	return m_pimpl->Open(
		[&](Qty qty) -> OrderId {
			return DoOpenImmediatelyOrCancel(qty, price, params);
		});
}

OrderId Position::OpenAtMarketPriceImmediatelyOrCancel() {
	return OpenAtMarketPriceImmediatelyOrCancel(defaultOrderParams);
}

OrderId Position::OpenAtMarketPriceImmediatelyOrCancel(
			const OrderParams &params) {
	return m_pimpl->Open(
		[&](const Qty &qty) {
			return DoOpenAtMarketPriceImmediatelyOrCancel(qty, params);
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

OrderId Position::CloseImmediatelyOrCancel(
			const CloseType &closeType,
			const ScaledPrice &price) {
	return CloseImmediatelyOrCancel(closeType, price, defaultOrderParams);
}

OrderId Position::CloseImmediatelyOrCancel(
			const CloseType &closeType,
			const ScaledPrice &price,
			const OrderParams &params) {
	return m_pimpl->Close(
		closeType,
		[&](const Qty &qty) {
			return DoCloseImmediatelyOrCancel(qty, price, params);
		});
}

OrderId Position::CloseAtMarketPriceImmediatelyOrCancel(
			const CloseType &closeType) {
	return CloseAtMarketPriceImmediatelyOrCancel(closeType, defaultOrderParams);
}

OrderId Position::CloseAtMarketPriceImmediatelyOrCancel(
			const CloseType &closeType,
			const OrderParams &params) {
	return m_pimpl->Close(
		closeType,
		[&](const Qty &qty) {
			return DoCloseAtMarketPriceImmediatelyOrCancel(qty, params);
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
			TradeSystem &tradeSystem,
			Security &security,
			const Currency &currency,
			Qty qty,
			ScaledPrice startPrice,
			const TimeMeasurement::Milestones &timeMeasurement)
		: Position(
			strategy,
			tradeSystem,
			security,
			currency,
			qty,
			startPrice,
			timeMeasurement) {
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

OrderId LongPosition::DoOpenAtMarketPrice(Qty qty, const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().BuyAtMarketPrice(
		GetSecurity(),
		GetCurrency(),
		qty,
		params,
		boost::bind(
			&LongPosition::UpdateOpening,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId LongPosition::DoOpen(Qty qty, ScaledPrice price, const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().Buy(
		GetSecurity(),
		GetCurrency(),
		qty,
		price,
		params,
		boost::bind(
			&LongPosition::UpdateOpening,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId LongPosition::DoOpenAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice,
			const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().BuyAtMarketPriceWithStopPrice(
		GetSecurity(),
		GetCurrency(),
		qty,
		stopPrice,
		params,
		boost::bind(
			&LongPosition::UpdateOpening,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId LongPosition::DoOpenImmediatelyOrCancel(
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().BuyImmediatelyOrCancel(
		GetSecurity(),
		GetCurrency(),
		qty,
		price,
		params,
		boost::bind(
			&LongPosition::UpdateOpening,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId LongPosition::DoOpenAtMarketPriceImmediatelyOrCancel(
			const Qty &qty,
			const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().BuyAtMarketPriceImmediatelyOrCancel(
		GetSecurity(),
		GetCurrency(),
		qty,
		params,
		boost::bind(
			&LongPosition::UpdateOpening,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId LongPosition::DoCloseAtMarketPrice(Qty qty, const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().SellAtMarketPrice(
		GetSecurity(),
		GetCurrency(),
		qty,
		params,
		boost::bind(
			&LongPosition::UpdateClosing,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId LongPosition::DoClose(Qty qty, ScaledPrice price, const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().Sell(
		GetSecurity(),
		GetCurrency(),
		qty,
		price,
		params,
		boost::bind(
			&LongPosition::UpdateClosing,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId LongPosition::DoCloseAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice,
			const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().SellAtMarketPriceWithStopPrice(
		GetSecurity(),
		GetCurrency(),
		qty,
		stopPrice,
		params,
		boost::bind(
			&LongPosition::UpdateClosing,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId LongPosition::DoCloseImmediatelyOrCancel(
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetTradeSystem().SellImmediatelyOrCancel(
		GetSecurity(),
		GetCurrency(),
		qty,
		price,
		params,
		boost::bind(
			&LongPosition::UpdateClosing,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId LongPosition::DoCloseAtMarketPriceImmediatelyOrCancel(
			const Qty &qty,
			const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetTradeSystem().SellAtMarketPriceImmediatelyOrCancel(
		GetSecurity(),
		GetCurrency(),
		qty,
		params,
		boost::bind(
			&LongPosition::UpdateClosing,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

//////////////////////////////////////////////////////////////////////////

ShortPosition::ShortPosition(
			Strategy &strategy,
			TradeSystem &tradeSystem,
			Security &security,
			const Currency &currency,
			Qty qty,
			ScaledPrice startPrice,
			const TimeMeasurement::Milestones &timeMeasurement)
		: Position(
			strategy,
			tradeSystem,
			security,
			currency,
			abs(qty),
			startPrice,
			timeMeasurement) {
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

OrderId ShortPosition::DoOpenAtMarketPrice(Qty qty, const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().SellAtMarketPrice(
		GetSecurity(),
		GetCurrency(),
		qty,
		params,
		boost::bind(
			&ShortPosition::UpdateOpening,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId ShortPosition::DoOpen(Qty qty, ScaledPrice price, const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().Sell(
		GetSecurity(),
		GetCurrency(),
		qty,
		price,
		params,
		boost::bind(
			&ShortPosition::UpdateOpening,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId ShortPosition::DoOpenAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice,
			const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().SellAtMarketPriceWithStopPrice(
		GetSecurity(),
		GetCurrency(),
		qty,
		stopPrice,
		params,
		boost::bind(
			&ShortPosition::UpdateOpening,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId ShortPosition::DoOpenImmediatelyOrCancel(
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().SellImmediatelyOrCancel(
		GetSecurity(),
		GetCurrency(),
		qty,
		price,
		params,
		boost::bind(
			&ShortPosition::UpdateOpening,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId ShortPosition::DoOpenAtMarketPriceImmediatelyOrCancel(
			const Qty &qty,
			const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().SellAtMarketPriceImmediatelyOrCancel(
		GetSecurity(),
		GetCurrency(),
		qty,
		params,
		boost::bind(
			&ShortPosition::UpdateOpening,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId ShortPosition::DoCloseAtMarketPrice(
			Qty qty,
			const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().BuyAtMarketPrice(
		GetSecurity(),
		GetCurrency(),
		qty,
		params,
		boost::bind(
			&ShortPosition::UpdateClosing,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId ShortPosition::DoClose(
			Qty qty,
			ScaledPrice price,
			const OrderParams &params) {
	return GetTradeSystem().Buy(
		GetSecurity(),
		GetCurrency(),
		qty,
		price,
		params,
		boost::bind(
			&ShortPosition::UpdateClosing,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId ShortPosition::DoCloseAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice,
			const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().BuyAtMarketPriceWithStopPrice(
		GetSecurity(),
		GetCurrency(),
		qty,
		stopPrice,
		params,
		boost::bind(
			&ShortPosition::UpdateClosing,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId ShortPosition::DoCloseImmediatelyOrCancel(
			const Qty &qty,
			const ScaledPrice &price,
			const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().BuyImmediatelyOrCancel(
		GetSecurity(),
		GetCurrency(),
		qty,
		price,
		params,
		boost::bind(
			&ShortPosition::UpdateClosing,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

OrderId ShortPosition::DoCloseAtMarketPriceImmediatelyOrCancel(
			const Qty &qty,
			const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetTradeSystem().BuyAtMarketPriceImmediatelyOrCancel(
		GetSecurity(),
		GetCurrency(),
		qty,
		params,
		boost::bind(
			&ShortPosition::UpdateClosing,
			shared_from_this(),
			_1,
			_2,
			_3,
			_4,
			_5));
}

//////////////////////////////////////////////////////////////////////////
