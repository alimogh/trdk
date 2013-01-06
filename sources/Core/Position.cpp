/**************************************************************************
 *   Created: 2012/07/08 04:07:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Position.hpp"
#include "Strategy.hpp"
#include "Settings.hpp"

using namespace Trader;
using namespace Trader::Lib;

//////////////////////////////////////////////////////////////////////////

Position::LogicError::LogicError(const char *what) throw()
		: Trader::Lib::LogicError(what) {
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

		volatile OrderId orderId;
		Time time;
		struct Price {
			volatile long long total;
			volatile long count;
			Price()
					: total(0),
					count(0) {
				//...//
			}
		} price;
		volatile Qty qty;
		volatile ScaledPrice comission;

		volatile bool hasOrder;

		DynamicData()
				: orderId(nOrderId),
				qty(0),
				comission(0),
				hasOrder(false) {
			//...//
		}

	};

public:

	Position &m_position;

	mutable Mutex m_mutex;

	mutable StateUpdateSignal m_stateUpdateSignal;

	boost::shared_ptr<Security> m_security;

	volatile long m_planedQty;

	const ScaledPrice m_openStartPrice;
	DynamicData m_opened;

	ScaledPrice m_closeStartPrice;
	DynamicData m_closed;

	volatile long m_closeType;

	volatile long m_isStarted;

	volatile long m_isError;

	volatile long m_isCanceled;
	boost::function<void ()> m_cancelMethod;

	const std::string m_tag;

	explicit Implementation(
				Position &position,
				boost::shared_ptr<Security> security,
				Qty qty,
				ScaledPrice startPrice,
				const std::string &tag)
			: m_position(position),
			m_security(security),
			m_planedQty(qty),
			m_openStartPrice(startPrice),
			m_closeStartPrice(0),
			m_closeType(CLOSE_TYPE_NONE),
			m_isError(false),
			m_isCanceled(0),
			m_tag(tag) {
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

		WriteLock lock(m_mutex);

		Assert(!m_position.IsOpened());
		Assert(!m_position.IsClosed());
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

		bool isCompleted = false;
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
				Interlocking::Exchange(
					m_opened.price.total,
					m_opened.price.total + m_security->ScalePrice(avgPrice));
				Interlocking::Increment(m_opened.price.count);
				Interlocking::Exchange(m_opened.qty, m_opened.qty + filled);
				AssertGt(m_opened.qty, 0);
				ReportOpeningUpdate("filled", orderStatus);
				isCompleted = remaining == 0;
				break;
			case TradeSystem::ORDER_STATUS_INACTIVE:
			case TradeSystem::ORDER_STATUS_ERROR:
				ReportOpeningUpdate("error", orderStatus);
				Interlocking::Exchange(m_isError, true);
				isCompleted = true;
				break;
			case TradeSystem::ORDER_STATUS_CANCELLED:
				if (m_opened.qty > 0) {
					ReportOpeningUpdate("canceled", orderStatus);
				}
				isCompleted = true;
				break;
		}
	
		AssertLe(m_opened.qty, m_planedQty);
		Assert(m_opened.time.is_not_a_date_time());

		if (!isCompleted) {
			return;
		}

		if (m_position.GetOpenedQty() > 0) {
			try {
				m_opened.time = !m_security->GetSettings().IsReplayMode()
					?	boost::get_system_time()
					:	m_security->GetLastMarketDataTime();
			} catch (...) {
				AssertFailNoException();
			}
		}

		Interlocking::Exchange(m_opened.hasOrder, false);

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

		bool isCompleted = false;
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
				Interlocking::Exchange(
					m_closed.price.total,
					m_closed.price.total + m_security->ScalePrice(avgPrice));
				Interlocking::Increment(m_closed.price.count);
				Interlocking::Exchange(m_closed.qty, m_closed.qty + filled);
				AssertGt(m_closed.qty, 0);
				isCompleted = remaining == 0;
				ReportClosingUpdate("filled", orderStatus);
				break;
			case TradeSystem::ORDER_STATUS_INACTIVE:
			case TradeSystem::ORDER_STATUS_ERROR:
				Log::Error(
					"Position CLOSE error: symbol: \"%1%\", strategy %2%"
						", trade system state: %3%, orders ID: %4%->%5%, qty: %6%->%7%.",
					m_position.GetSecurity(),
					m_tag,
					orderStatus,
					m_opened.orderId,
					m_closed.orderId,	
					m_opened.qty,
					m_closed.qty);
				ReportClosingUpdate("error", orderStatus);
				Interlocking::Exchange(m_isError, true);
				isCompleted = true;
				break;
			case TradeSystem::ORDER_STATUS_CANCELLED:
				if (m_closed.qty > 0) {
					ReportClosingUpdate("canceled", orderStatus);
				}
				isCompleted = true;
				break;
		}
	
		AssertLe(m_opened.qty, m_planedQty);
		Assert(m_closed.time.is_not_a_date_time());

		if (!isCompleted) {
			return;
		}

		if (m_position.GetActiveQty() == 0) {
			try {
				m_closed.time = !m_security->GetSettings().IsReplayMode()
					?	boost::get_system_time()
					:	m_security->GetLastMarketDataTime();
			} catch (...) {
				Interlocking::Exchange(m_isError, true);
				AssertFailNoException();
			}
		}

		Interlocking::Exchange(m_closed.hasOrder, false);

		if (CancelIfSet()) {
			return;
		}

		lock.unlock();
		SignalUpdate();		

	}

	void SignalUpdate() {
		const auto refCopy = m_position.shared_from_this();
		m_stateUpdateSignal();
	}

public:

	bool CancelIfSet() throw() {
		if (	m_position.IsClosed()
				|| Interlocking::CompareExchange(m_isCanceled, 2, 1) != 1) {
			return false;
		}
		Log::Trading(
			"position",
			"%1% %2% close-cancel-post %3% qty=%4%->%5% price=market order-id=%6%->%7%"
				" has-orders=%8%/%9% is-error=%10%",
			m_security->GetSymbol(),
			m_position.GetTypeStr(),
			m_tag,
			m_position.GetOpenedQty(),
			m_position.GetClosedQty(),
			m_position.GetOpenOrderId(),
			m_position.GetCloseOrderId(),
			m_position.HasActiveOpenOrders(),
			m_position.HasActiveCloseOrders(),
			m_isError);
		try {
			m_cancelMethod();
			return true;
		} catch (...) {
			Interlocking::Exchange(m_isError, true);
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
			Log::Trading(
				"position",
				"%1% %2% open-%3% %4% qty=%5%->%6% price=%7%->%8% order-id=%9%"
					" order-status=%10% is-error=%11%",
				m_security->GetSymbol(),
				m_position.GetTypeStr(),
				eventDesc,
				m_tag,
				m_position.GetPlanedQty(),
				m_position.GetOpenedQty(),
				m_security->DescalePrice(m_position.GetOpenStartPrice()),
				m_security->DescalePrice(m_position.GetOpenPrice()),
				m_position.GetOpenOrderId(),
				m_security->GetTradeSystem().GetStringStatus(orderStatus),
				m_isError);
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
			Log::Trading(
				"position",
				"%1% %2% close-%3% %4% qty=%5%->%6% price=%7% order-id=%8%->%9%"
					" order-status=%10% is-error=%11%",
				m_position.GetSecurity().GetSymbol(),
				m_position.GetTypeStr(),
				eventDesc,
				m_tag,
				m_position.GetOpenedQty(),
				m_position.GetClosedQty(),
				m_position.GetSecurity().DescalePrice(m_position.GetClosePrice()),
				m_position.GetOpenOrderId(),
				m_position.GetCloseOrderId(),
				m_position.GetSecurity().GetTradeSystem().GetStringStatus(orderStatus),
				m_isError);
		} catch (...) {
			AssertFailNoException();
		}
	}

public:

	template<typename OpenImpl>
	OrderId Open(const OpenImpl &openImpl) {
		
		using namespace Interlocking;
		
		const WriteLock lock(m_mutex);
		if (m_position.IsStarted()) {
			throw AlreadyStartedError();
		}
		
		Assert(!m_position.IsOpened());
		Assert(!m_position.IsError());
		Assert(!m_position.IsClosed());
		Assert(!m_position.IsCompleted());
		Assert(!m_position.HasActiveOrders());
		
		const auto orderId = openImpl(m_position.GetNotOpenedQty());
		Exchange(m_opened.hasOrder, true);
		Verify(Exchange(m_opened.orderId, orderId) == nOrderId);
		Verify(Exchange(m_isStarted, true) == false);
	
		return orderId;
	
	}

	template<typename CloseImpl>
	OrderId CloseUnsafe(CloseType closeType, const CloseImpl &closeImpl) {
		
		using namespace Interlocking;

		if (!m_position.IsOpened()) {
			throw NotOpenedError();
		} else if (m_position.HasActiveCloseOrders() || m_position.IsClosed()) {
			throw AlreadyClosedError();
		}

		Assert(m_position.IsStarted());
		Assert(!m_position.IsError());
		Assert(!m_position.HasActiveOrders());
		Assert(!m_position.IsCompleted());

		const auto orderId = closeImpl(m_position.GetActiveQty());
		Exchange(m_closeType, closeType);
		Exchange(m_closed.hasOrder, true);
		Exchange(m_closed.orderId, orderId);

		return orderId;

	}

	template<typename CloseImpl>
	OrderId Close(CloseType closeType, const CloseImpl &closeImpl) {
		const Implementation::WriteLock lock(m_mutex);
		return CloseUnsafe(closeType, closeImpl);
	}

	bool CancelAllOrders() {
		bool isCanceled = false;
		if (m_opened.hasOrder) {
			m_security->CancelOrder(m_opened.orderId);
			isCanceled = true;
		}
		if (m_closed.hasOrder) {
			Assert(!isCanceled);
			m_security->CancelOrder(m_closed.orderId);
			isCanceled = true;
		}
		return isCanceled;
	}

};


//////////////////////////////////////////////////////////////////////////

Position::Position(
			Strategy &strategy,
			Qty qty,
			ScaledPrice startPrice) {
	m_pimpl = new Implementation(
		*this,
		strategy.GetSecurity().shared_from_this(),
		qty,
		startPrice,
		strategy.GetTag());
	AssertGt(m_pimpl->m_planedQty, 0);
	strategy.Register(*this);
}

Position::~Position() {
	delete m_pimpl;
}

const Security & Position::GetSecurity() const throw() {
	return const_cast<Position *>(this)->GetSecurity();
}
Security & Position::GetSecurity() throw() {
	return *m_pimpl->m_security;
}

Position::CloseType Position::GetCloseType() const throw() {
	return CloseType(m_pimpl->m_closeType);
}

bool Position::IsOpened() const throw() {
	return !HasActiveOpenOrders() && GetOpenedQty() > 0;
}
bool Position::IsClosed() const throw() {
	return
		!HasActiveCloseOrders()
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
	Assert(GetOpenedQty() <= GetPlanedQty());
	return GetPlanedQty() - GetOpenedQty();
}

Qty Position::GetActiveQty() const throw() {
	Assert(GetOpenedQty() >= GetClosedQty());
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
	return m_pimpl->Open(
		[this](Qty qty) -> OrderId {
			return DoOpenAtMarketPrice(qty);
		});
}

OrderId Position::Open(ScaledPrice price) {
	return m_pimpl->Open(
		[this, price](Qty qty) -> OrderId {
			return DoOpen(qty, price);
		});
}

OrderId Position::OpenAtMarketPriceWithStopPrice(ScaledPrice stopPrice) {
	return m_pimpl->Open(
		[this, stopPrice](Qty qty) -> OrderId {
			return DoOpenAtMarketPriceWithStopPrice(qty, stopPrice);
		});
}

OrderId Position::OpenOrCancel(ScaledPrice price) {
	return m_pimpl->Open(
		[this, price](Qty qty) -> OrderId {
			return DoOpenOrCancel(qty, price);
		});
}

OrderId Position::CloseAtMarketPrice(CloseType closeType) {
	return m_pimpl->Close(
		closeType,
		[this](Qty qty) -> OrderId {
			return DoCloseAtMarketPrice(qty);
		});
}

OrderId Position::Close(CloseType closeType, ScaledPrice price) {
	return m_pimpl->Close(
		closeType,
		[this, price](Qty qty) -> OrderId {
			return DoClose(qty, price);
		});
}

OrderId Position::CloseAtMarketPriceWithStopPrice(
			CloseType closeType,
			ScaledPrice stopPrice) {
	return m_pimpl->Close(
		closeType,
		[this, stopPrice](Qty qty) -> OrderId {
			return DoCloseAtMarketPriceWithStopPrice(qty, stopPrice);
		});
}

OrderId Position::CloseOrCancel(CloseType closeType, ScaledPrice price) {
	return m_pimpl->Close(
		closeType,
		[this, price](Qty qty) -> OrderId {
			return DoCloseOrCancel(qty, price);
		});
}

bool Position::CancelAtMarketPrice(CloseType closeType) {
	const Implementation::WriteLock lock(m_pimpl->m_mutex);
	if (IsCanceled()) {
		return false;
	}
	Log::Trading(
		"position",
		"%1% %2% close-cancel-pre %3% qty=%4%->%5% price=market order-id=%6%->%7%"
			" has-orders=%8%/%9% is-error=%10%",
		GetSecurity().GetSymbol(),
		GetTypeStr(),
		m_pimpl->m_tag,
		GetOpenedQty(),
		GetClosedQty(),
		GetOpenOrderId(),
		GetCloseOrderId(),
		HasActiveOpenOrders(),
		HasActiveCloseOrders(),
		m_pimpl->m_isError);
	if (IsClosed() || (!IsOpened() && !HasActiveOpenOrders())) {
		return false;
	}
	const auto cancelMethodImpl = [this](Qty qty) -> OrderId {
			return DoCloseAtMarketPrice(qty);
		};
	boost::function<void ()> cancelMethod = boost::bind(
		&Implementation::CloseUnsafe<decltype(cancelMethodImpl)>,
		m_pimpl,
		closeType,
		cancelMethodImpl);
	if (m_pimpl->CancelAllOrders()) {
		Assert(HasActiveOrders());
		cancelMethod.swap(m_pimpl->m_cancelMethod);
	} else {
		Assert(!HasActiveOrders());
		cancelMethod();
	}
	Verify(Interlocking::Exchange(m_pimpl->m_isCanceled, 1) == 0);
	return true;
}

bool Position::CancelAllOrders() {
	const Implementation::WriteLock lock(m_pimpl->m_mutex);
	return m_pimpl->CancelAllOrders();
}

//////////////////////////////////////////////////////////////////////////

LongPosition::LongPosition(
			Strategy &strategy,
			Qty qty,
			ScaledPrice startPrice)
		: Position(strategy, qty, startPrice) {
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

OrderId LongPosition::DoOpenAtMarketPrice(Qty qty) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().BuyAtMarketPrice(qty, *this);
}

OrderId LongPosition::DoOpen(Qty qty, ScaledPrice price) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().Buy(qty, price, *this);
}

OrderId LongPosition::DoOpenAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().BuyAtMarketPriceWithStopPrice(qty, stopPrice, *this);
}

OrderId LongPosition::DoOpenOrCancel(Qty qty, ScaledPrice price) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().BuyOrCancel(qty, price, *this);
}

OrderId LongPosition::DoCloseAtMarketPrice(Qty qty) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().SellAtMarketPrice(qty, *this);
}

OrderId LongPosition::DoClose(Qty qty, ScaledPrice price) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().Sell(qty, price, *this);
}

OrderId LongPosition::DoCloseAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().SellAtMarketPriceWithStopPrice(qty, stopPrice, *this);
}

OrderId LongPosition::DoCloseOrCancel(Qty qty, ScaledPrice price) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().SellOrCancel(qty, price, *this);
}

//////////////////////////////////////////////////////////////////////////

ShortPosition::ShortPosition(
			Strategy &strategy,
			Qty qty,
			ScaledPrice startPrice)
		: Position(strategy, qty, startPrice) {
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

OrderId ShortPosition::DoOpenAtMarketPrice(Qty qty) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().SellAtMarketPrice(qty, *this);
}

OrderId ShortPosition::DoOpen(Qty qty, ScaledPrice price) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().Sell(qty, price, *this);
}

OrderId ShortPosition::DoOpenAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().SellAtMarketPriceWithStopPrice(qty, stopPrice, *this);
}

OrderId ShortPosition::DoOpenOrCancel(Qty qty, ScaledPrice price) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().SellOrCancel(qty, price, *this);
}

OrderId ShortPosition::DoCloseAtMarketPrice(Qty qty) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().BuyAtMarketPrice(qty, *this);
}

OrderId ShortPosition::DoClose(Qty qty, ScaledPrice price) {
	Assert(qty > 0);
	return GetSecurity().Buy(qty, price, *this);
}

OrderId ShortPosition::DoCloseAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().BuyAtMarketPriceWithStopPrice(qty, stopPrice, *this);
}

OrderId ShortPosition::DoCloseOrCancel(Qty qty, ScaledPrice price) {
	Assert(IsOpened());
	Assert(!IsClosed());
	Assert(qty > 0);
	return GetSecurity().BuyOrCancel(qty, price, *this);
}

//////////////////////////////////////////////////////////////////////////
