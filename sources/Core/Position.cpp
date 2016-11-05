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
#include "DropCopy.hpp"
#include "Strategy.hpp"
#include "Settings.hpp"
#include "TradingLog.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;
namespace uuids = boost::uuids;
namespace sig = boost::signals2;

//////////////////////////////////////////////////////////////////////////

namespace {
	
	const OrderParams defaultOrderParams;

	class UuidGenerator : private boost::noncopyable {
	public:
		uuids::uuid operator ()() {
			return m_generator();
		}
	private:
		uuids::random_generator m_generator;
	} generateUuid;

}

//////////////////////////////////////////////////////////////////////////

Position::Exception::Exception(const char *what) noexcept
	: Lib::Exception(what) {
	//...//
}
		
Position::AlreadyStartedError::AlreadyStartedError() noexcept
	: Exception("Position already started") {
	//...//
}

Position::NotOpenedError::NotOpenedError() noexcept
	: Exception("Position not opened") {
	//...//
}
		
Position::AlreadyClosedError::AlreadyClosedError() noexcept
	: Exception("Position already closed") {
	//...//
}

//////////////////////////////////////////////////////////////////////////

class Position::Implementation : private boost::noncopyable {

public:

	template<typename SlotSignature>
	struct SignalTrait {
		typedef sig::signal<
				SlotSignature,
				sig::optional_last_value<
					typename boost::function_traits<
							SlotSignature>
						::result_type>,
				int,
				std::less<int>,
				boost::function<SlotSignature>,
				typename sig::detail::extended_signature<
						boost::function_traits<SlotSignature>::arity,
						SlotSignature>
					::function_type,
				sig::dummy_mutex>
			Signal;
	};
	typedef SignalTrait<StateUpdateSlotSignature>::Signal StateUpdateSignal;

	struct Order {

		bool isActive;

		const double price;
		const Qty qty;

		uuids::uuid uuid;

		OrderId id;

		Qty executedQty;

		explicit Order(double price, const Qty &qty, const uuids::uuid &&uuid)
			: isActive(true)
			, price(price)
			, qty(qty)
			, uuid(std::move(uuid))
			, id(0)
			, executedQty(0) {
			//...//
		}

	};

	struct CloseOrder : public Order {
		
		CloseType closeType;

		explicit CloseOrder(
				double price,
				const Qty &qty,
				const CloseType &closeType,
				const uuids::uuid &&uuid)
			: Order(price, qty, std::move(uuid))
			, closeType(closeType) {
			//...//
		}

	};

	template<typename Order>
	struct DirectionData { 
		
		ScaledPrice startPrice;

		pt::ptime time;
		
		ScaledPrice volume;
		Qty qty;
		size_t numberOfTrades;
		ScaledPrice lastTradePrice;
		std::vector<Order> orders;

		explicit DirectionData(const ScaledPrice &startPrice)
			: startPrice(startPrice)
			, volume(0)
			, qty(0)
			, numberOfTrades(0)
			, lastTradePrice(0) {
			//...//
		}

		bool HasActiveOrders() const {
			return !orders.empty() && orders.back().isActive;
		}

		void OnNewTrade(const TradingSystem::TradeInfo &trade) {
			AssertLt(0, trade.price);
			AssertLt(0, trade.qty);
			Assert(!orders.empty());
			auto &order = orders.back();
			order.executedQty += trade.qty;
			volume += ScaledPrice(trade.price * trade.qty);
			qty += trade.qty;
			++numberOfTrades;
			lastTradePrice = trade.price;
		}

	};

	typedef DirectionData<Order> OpenData;
	typedef DirectionData<CloseOrder> CloseData;

	enum CancelState {
		CANCEL_STATE_NOT_CANCELED,
		CANCEL_STATE_SCHEDULED,
		CANCEL_STATE_CANCELED,
	};

public:

	Position &m_self;

	TradingSystem &m_tradingSystem;

	mutable StateUpdateSignal m_stateUpdateSignal;

	Strategy &m_strategy;
	const uuids::uuid m_operationId;
	const int64_t m_subOperationId;
	bool m_isRegistered;
	Security &m_security;
	const Currency m_currency;

	Qty m_planedQty;

	boost::optional<ContractExpiration> m_expiration;

	bool m_isMarketAsCompleted;

	bool m_isError;
	bool m_isInactive;

	CancelState m_cancelState;
	boost::function<void ()> m_cancelMethod;

	TimeMeasurement::Milestones m_timeMeasurement;

	OpenData m_open;
	CloseData m_close;

	explicit Implementation(
			Position &position,
			TradingSystem &tradingSystem,
			Strategy &strategy,
			const uuids::uuid &operationId,
			int64_t subOperationId,
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &startPrice,
			const TimeMeasurement::Milestones &timeMeasurement)
		: m_self(position)
		, m_tradingSystem(tradingSystem)
		, m_strategy(strategy)
		, m_operationId(operationId)
		, m_subOperationId(subOperationId)
		, m_isRegistered(false)
		, m_security(security)
		, m_currency(currency)
		, m_planedQty(qty)
		, m_isMarketAsCompleted(false)
		, m_isError(false)
		, m_isInactive(false)
		, m_cancelState(CANCEL_STATE_NOT_CANCELED)
		, m_timeMeasurement(timeMeasurement)
		, m_open(startPrice)
		, m_close(0) {
		AssertLt(0, m_planedQty);
	}

public:

	void UpdateOpening(
			const OrderId &orderId,
			const std::string &tradingSystemOrderId,
			const OrderStatus &orderStatus,
			const Qty &remainingQty,
			const TradingSystem::TradeInfo *trade) {

		auto lock = m_strategy.LockForOtherThreads();

		Assert(!m_self.IsOpened());
		Assert(!m_self.IsClosed());
		Assert(!m_self.IsCompleted());
		Assert(!m_open.orders.empty());
		Assert(m_close.orders.empty());
		Assert(m_close.time.is_not_a_date_time());

		Order &order = m_open.orders.back();
		AssertGe(order.id, orderId);
		AssertGe(order.qty, remainingQty);
		AssertLe(order.qty, m_planedQty);
		Assert(order.isActive);
		if (order.id != orderId) {
			throw Exception("Unknown open order ID");
		}

		static_assert(numberOfOrderStatuses == 9, "List changed.");
		switch (orderStatus) {
			default:
				AssertFail("Unknown open order status");
				throw Exception("Unknown open order status");
			case ORDER_STATUS_SENT:
			case ORDER_STATUS_REQUESTED_CANCEL:
				AssertFail("Status can be set only by this object.");
				throw Exception("Status can be set only by this object");
			case ORDER_STATUS_SUBMITTED:
				AssertEq(0, order.executedQty);
				AssertEq(0, m_open.volume);
				Assert(!trade);
				AssertLt(0, remainingQty);
				return;
			case ORDER_STATUS_FILLED:
			case ORDER_STATUS_FILLED_PARTIALLY:
				Assert(trade);
				if (!trade) {
					throw Exception("Filled order has no trade information");
				}
				AssertEq(
					order.executedQty + trade->qty + remainingQty,
					order.qty);
				m_open.OnNewTrade(*trade);
				ReportOpeningUpdate(
					"filled",
					tradingSystemOrderId,
					orderStatus);
				CopyTrade(
					trade->id,
					m_security.DescalePrice(trade->price),
					trade->qty,
					true);
				order.isActive = remainingQty > 0;
				break;
			case ORDER_STATUS_INACTIVE:
				order.isActive = false;
				ReportOpeningUpdate(
					"inactive",
					tradingSystemOrderId,
					orderStatus);
				m_isInactive = true;
				break;
			case ORDER_STATUS_ERROR:
				order.isActive = false;
				ReportOpeningUpdate(
					"error",
					tradingSystemOrderId,
					orderStatus);
				m_isError = true;
				break;
			case ORDER_STATUS_CANCELLED:
			case ORDER_STATUS_REJECTED:
				order.isActive = false;
				ReportOpeningUpdate(
					TradingSystem::GetStringStatus(orderStatus),
					tradingSystemOrderId,
					orderStatus);
				break;
		}

		CopyOrder(&tradingSystemOrderId, true, orderStatus);

		AssertEq(order.isActive, m_open.HasActiveOrders());
		if (!order.isActive) {
			
			AssertLe(order.executedQty, order.qty);

			if (order.qty && m_open.time.is_not_a_date_time()) {
				m_open.time = m_security.GetContext().GetCurrentTime();
			}

			if (CancelIfSet()) {
				return;
			}

		}

		lock.unlock();

		if (!order.isActive) {
			SignalUpdate();
		}

	}

	void UpdateClosing(
			const OrderId &orderId,
			const std::string &tradingSystemOrderId,
			const OrderStatus &orderStatus,
			const Qty &remainingQty,
			const TradingSystem::TradeInfo *trade) {

		auto lock = m_strategy.LockForOtherThreads();

		Assert(m_self.IsOpened());
		Assert(!m_self.IsClosed());
		Assert(!m_self.IsCompleted());
		Assert(!m_open.time.is_not_a_date_time());
		Assert(m_close.time.is_not_a_date_time());
		Assert(!m_open.orders.empty());
		Assert(!m_open.HasActiveOrders());
		Assert(!m_close.orders.empty());

		CloseOrder &order = m_close.orders.back();
		AssertLt(0, order.qty);
		AssertGe(order.qty, remainingQty);
		AssertLe(order.executedQty, order.qty);
		AssertEq(order.id, orderId);
		Assert(order.isActive);
		if (order.id != orderId) {
			throw Exception("Unknown open order ID");
		}

		static_assert(numberOfOrderStatuses == 9, "List changed.");
		switch (orderStatus) {
			default:
				AssertFail("Unknown order status.");
				throw Exception("Unknown close order status");
			case ORDER_STATUS_SENT:
			case ORDER_STATUS_REQUESTED_CANCEL:
				AssertFail("Status can be set only by this object.");
				throw Exception("Status can be set only by this object");
			case ORDER_STATUS_SUBMITTED:
				AssertEq(0, order.executedQty);
				AssertLt(0, remainingQty);
				return;
			case ORDER_STATUS_FILLED:
			case ORDER_STATUS_FILLED_PARTIALLY:
				Assert(trade);
				if (!trade) {
					throw Exception("Filled order has no trade information");
				}
				AssertEq(
					order.executedQty + trade->qty + remainingQty,
					order.qty);
				m_close.OnNewTrade(*trade);
				ReportClosingUpdate(
					"filled",
					tradingSystemOrderId,
					orderStatus);
				CopyTrade(
					trade->id,
					m_security.DescalePrice(trade->price),
					trade->qty,
					false);
				if (remainingQty != 0) {
					return;
				}
				break;
			case ORDER_STATUS_INACTIVE:
				ReportClosingUpdate("error", tradingSystemOrderId, orderStatus);
				m_isInactive = true;
				break;
			case ORDER_STATUS_ERROR:
				ReportClosingUpdate("error", tradingSystemOrderId, orderStatus);
				m_isError = true;
				break;
			case ORDER_STATUS_CANCELLED:
			case ORDER_STATUS_REJECTED:
				ReportClosingUpdate(
					TradingSystem::GetStringStatus(orderStatus),
					tradingSystemOrderId,
					orderStatus);
				break;
		}
	
		AssertLe(order.executedQty, order.qty);
		Assert(m_close.time.is_not_a_date_time());

		if (!m_self.GetActiveQty()) {
			Assert(m_close.time.is_not_a_date_time());
			m_close.time = m_security.GetContext().GetCurrentTime();
		}

		order.isActive = false;
		AssertEq(order.isActive, m_close.HasActiveOrders());

		CopyOrder(&tradingSystemOrderId, false, orderStatus);

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

	bool CancelIfSet() noexcept {
		
		if (m_self.IsClosed() || m_cancelState != CANCEL_STATE_SCHEDULED) {
			return false;
		}

		m_cancelState = CANCEL_STATE_CANCELED;

		try {
			m_cancelMethod();
			return true;
		} catch (...) {
			m_isError = true;
			AssertFailNoException();
			return false;
		}

	}

	void ReportOpeningStart(const char *eventDesc) const noexcept {
		Assert(!m_open.orders.empty());
		m_strategy.GetTradingLog().Write(
			"order\tpos=%1%\torder=%2%\t%3%\t%4%\t%5%\t%6%.%7%"
				"\tprice=%8$.8f\t%9%\tqty=%10$.8f",
			[this, eventDesc](TradingRecord &record) {
				record
					% m_operationId // 1
					% m_open.orders.back().uuid // 2
					% eventDesc // 3
					% m_self.GetTypeStr() // 4
					% m_security.GetSymbol().GetSymbol() // 5
					% m_self.GetTradingSystem().GetTag() // 6
					% m_tradingSystem.GetMode() // 7
					% m_security.DescalePrice(m_open.startPrice) // 8
					% m_self.GetCurrency() // 9
					% m_self.GetPlanedQty(); // 10 and last
			});
	}

	void ReportOpeningUpdate(
			const char *eventDesc,
			const std::string &tsOrderId,
			const OrderStatus &orderStatus)
			const
			noexcept {
		Assert(!m_open.orders.empty());
		m_strategy.GetTradingLog().Write(
			"order\t%1%\tpos=%1%\torder=%2%/%3%\topen-%4%->%5%\t%6%\t%7%\t%8%.%9%"
				"\tprice=%10$.8f->%11$.8f(%12$.8f)\t%13%\tqty=%14$.8f->%15$.8f",
			[this, eventDesc, &tsOrderId, &orderStatus](
					TradingRecord &record) {
				record
					% m_operationId // 1
					% m_open.orders.back().uuid // 2
					% tsOrderId // 3
					% eventDesc // 4
					% m_tradingSystem.GetStringStatus(orderStatus) // 5
					% m_self.GetTypeStr() // 6
					% m_security.GetSymbol().GetSymbol().c_str() // 7
					% m_self.GetTradingSystem().GetTag().c_str() // 8
					% m_tradingSystem.GetMode() // 9
					% m_security.DescalePrice(m_self.GetOpenStartPrice()); // 10
				if (m_self.GetOpenedQty()) {
					record
						% m_security.DescalePrice(m_open.lastTradePrice) // 11
						% m_security.DescalePrice(m_self.GetOpenAvgPrice()); // 12
				} else {
					record % '-' % '-'; // 11, 12
				}
				record
					% m_self.GetCurrency() // 13
					% m_self.GetPlanedQty() // 14
					% m_self.GetOpenedQty(); // 15 and last
					
			});
	}

	void ReportClosingStart(
			const char *eventDesc,
			const uuids::uuid &uuid,
			const Qty &maxQty)
			const
			noexcept {
		m_strategy.GetTradingLog().Write(
			"order\tpos=%1%\torder=%2%\tclose-%3%\t%4%\t%5%\t%6%.%7%"
				"\tprice=%8$.8f->%9$.8f(%10$.8f)\t%11%\tqty=(%12$.8f, %13$.8f)",
			[this, &uuid, eventDesc, &maxQty](TradingRecord &record) {
				record
					% m_operationId // 1
					% uuid // 2
					% eventDesc // 3
					% m_self.GetTypeStr() // 4
					% m_security.GetSymbol().GetSymbol().c_str() // 5
					% m_self.GetTradingSystem().GetTag().c_str() // 6
					% m_tradingSystem.GetMode() // 7
					% m_security.DescalePrice(m_open.lastTradePrice) // 8
					% m_security.DescalePrice(m_self.GetOpenAvgPrice()) // 9
					% m_security.DescalePrice(m_self.GetCloseStartPrice()) // 10
					% m_self.GetCurrency() // 11
					% maxQty // 12
					% m_self.GetActiveQty(); // 13 and last
			});
	}

	void ReportClosingUpdate(
			const char *eventDesc,
			const std::string &tsOrderId,
			const OrderStatus &orderStatus)
			const
			noexcept {
		Assert(!m_close.orders.empty());
		m_strategy.GetTradingLog().Write(
			"order\tpos=%1%\torder=%2%/%3%\tclose-%4%->%5%\t%6%\t%7%\t%8%.%9%"
				"\tprice=%10$.8f->%11$.8f(%12$.8f)\t%13%"
				"\tqty=%14$.8f-%15$.8f=%16$.8f",
			[this, eventDesc, &tsOrderId, &orderStatus](TradingRecord &record) {
				record
					% m_operationId // 1
					% m_close.orders.back().uuid // 2
					% tsOrderId // 3
					% eventDesc // 4
					% m_tradingSystem.GetStringStatus(orderStatus) // 5
					% m_self.GetTypeStr() // 6
					% m_security.GetSymbol().GetSymbol() // 7
					% m_self.GetTradingSystem().GetTag() // 8
					% m_tradingSystem.GetMode() // 9
					% m_security.DescalePrice(m_self.GetCloseStartPrice()); // 10
				if (m_self.GetClosedQty()) {
					record
						% m_security.DescalePrice(m_close.lastTradePrice) // 11
						% m_security.DescalePrice(m_self.GetCloseAvgPrice()); // 12
				} else {
					record % '-' % '-'; // 11, 12
				}
				record
					% m_self.GetCurrency() // 13
					% m_self.GetOpenedQty() // 14
					% m_self.GetClosedQty() // 15
					% m_self.GetActiveQty(); // 16 and last
			});
	}

public:

	template<typename OpenImpl>
	OrderId Open(
			const OpenImpl &openImpl,
			const TimeInForce &timeInForce,
			const OrderParams &orderParams,
			const ScaledPrice &price) {

		if (!m_security.IsOnline()) {
			throw Exception("Security is not online");
		}
		if (!m_security.IsTradingSessionOpened()) {
			throw Exception("Security trading session is closed");
		}
		if (!m_close.orders.empty()) {
			throw AlreadyStartedError();
		}

		Assert(!m_self.IsOpened());
		Assert(!m_self.IsError());
		Assert(!m_self.IsClosed());

		AssertGt(m_planedQty, m_open.qty);
		auto qty = m_planedQty - m_open.qty;
		const char *action = "open-pre";

		if (!m_isRegistered) {
			m_strategy.Register(m_self);
			// supporting prev. logic (when was m_strategy = nullptr),
			// don't know why set flag in other place.
		}

		m_open.orders.emplace_back(
			m_security.DescalePrice(price),
			qty,
			generateUuid());
		ReportOpeningStart(action);

		try {

			if (
					!orderParams.expiration
					&& !m_security.GetSymbol().IsExplicit()) {
				m_expiration = m_security.GetExpiration();
				OrderParams additionalOrderParams(orderParams);
				additionalOrderParams.expiration = &*m_expiration;
				m_open.orders.back().id = openImpl(qty, additionalOrderParams);
			} else {
				m_open.orders.back().id = openImpl(qty, orderParams);
			}

			m_isRegistered = true;	// supporting prev. logic
									// (when was m_strategy = nullptr),
									// don't know why set flag only here.

		} catch (...) {
			if (m_isRegistered) {
				m_strategy.Unregister(m_self);
			}
			try {
				CopyOrder(
					nullptr, // order ID (from trading system)
					true,
					ORDER_STATUS_ERROR,
					&timeInForce,
					&orderParams);
			} catch (...) {
				AssertFailNoException();
			}
			m_open.orders.pop_back();
			throw;
		}

		try {
			CopyOrder(
				nullptr, // order ID (from trading system)
				true,
				ORDER_STATUS_SENT,
				&timeInForce,
				&orderParams);
		} catch (...) {
			AssertFailNoException();
		}

		return m_open.orders.back().id;

	}

	template<typename CloseImpl>
	OrderId Close(
			const CloseType &closeType,
			const CloseImpl &closeImpl,
			const TimeInForce &timeInForce,
			const ScaledPrice &price,
			const Qty &maxQty,
			const OrderParams &orderParams) {
		return Close(
			closeType,
			closeImpl,
			timeInForce,
			price,
			maxQty,
			orderParams,
			generateUuid());
	}

	template<typename CloseImpl>
	OrderId Close(
			const CloseType &closeType,
			const CloseImpl &closeImpl,
			const TimeInForce &timeInForce,
			const ScaledPrice &price,
			const Qty &maxQty,
			const OrderParams &orderParams,
			const uuids::uuid &&uuid) {
		
		if (!m_self.IsOpened()) {
			throw NotOpenedError();
		} else if (m_self.HasActiveCloseOrders() || m_self.IsClosed()) {
			throw AlreadyClosedError();
		}

		Assert(m_isRegistered);
		Assert(m_self.IsStarted());
		Assert(!m_self.IsError());
		Assert(!m_self.HasActiveOrders());
		Assert(!m_self.IsCompleted());

		m_close.orders.emplace_back(
			m_security.DescalePrice(price),
			std::min<Qty>(maxQty, m_self.GetActiveQty()),
			closeType,
			std::move(uuid));
		Order &order = m_close.orders.back();

		ReportClosingStart("pre", order.uuid, maxQty);

		try {

			if (m_expiration && !orderParams.expiration) {
				OrderParams additionalOrderParams(orderParams);
				additionalOrderParams.expiration = &*m_expiration;
				order.id = closeImpl(order.qty, additionalOrderParams);
			} else {
				order.id = closeImpl(order.qty, orderParams);
			}

		} catch (...) {
			CopyOrder(
				nullptr, // order ID (from trading system)
				false,
				ORDER_STATUS_SENT,
				&timeInForce,
				&orderParams);
			m_close.orders.pop_back();
			throw;
		}

		try {
			CopyOrder(
				nullptr, // order ID (from trading system)
				false,
				ORDER_STATUS_SENT,
				&timeInForce,
				&orderParams);
		} catch (...) {
			AssertFailNoException();
		}

		return order.id;

	}

	template<typename CancelMethodImpl>
	bool CancelAtMarketPrice(
			const CloseType &closeType,
			const CancelMethodImpl &cancelMethodImpl,
			const TimeInForce &timeInForce,
			const OrderParams &orderParams) {
		
		if (m_self.IsCanceled()) {
			Assert(!m_self.HasActiveOpenOrders());
			Assert(!m_self.HasActiveCloseOrders());
			return false;
		}

		const auto &uuid = generateUuid();
		ReportClosingStart("cancel-pre", uuid, 0);

		if (	m_self.IsClosed()
				|| (!m_self.IsOpened() && !m_self.HasActiveOpenOrders())) {
			return false;
		}

		if (CancelAllOrders()) {
			boost::function<void ()> delayedCancelMethod
				= [
						this,
						&uuid,
						closeType,
						cancelMethodImpl,
						timeInForce,
						orderParams]() {
					if (!m_self.IsOpened() || m_self.IsClosed()) {
						SignalUpdate();
						return;
					}
					Close<CancelMethodImpl>(
						closeType,
						cancelMethodImpl,
						timeInForce,
						0,
						GetActiveQty(),
						orderParams,
						std::move(uuid));
				};
			Assert(m_self.HasActiveOrders());
			delayedCancelMethod.swap(m_cancelMethod);
			AssertEq(int(CANCEL_STATE_NOT_CANCELED), int(m_cancelState));
			m_cancelState = CANCEL_STATE_SCHEDULED;
		} else {
			Assert(!m_self.HasActiveOrders());
			Close<CancelMethodImpl>(
				closeType,
				cancelMethodImpl,
				timeInForce,
				0,
				m_self.GetActiveQty(),
				orderParams,
				std::move(uuid));
			AssertEq(int(CANCEL_STATE_NOT_CANCELED), int(m_cancelState));
			m_cancelState = CANCEL_STATE_CANCELED;
		}

		return true;
	
	}

	bool CancelAllOrders() {
		bool isCanceled = false;
		if (m_open.HasActiveOrders()) {
			Assert(!m_open.orders.empty());
			Assert(m_open.orders.back().isActive);
			m_strategy.GetTradingLog().Write(
				"order\tpos=%1%\torder=%2%/%3%\tcancel-all\topen-order"
					"\t%4%\t%5%\t%6%"
					,
				[this](TradingRecord &record) {
					record
						% m_operationId // 1
						% m_open.orders.back().uuid // 2
						% m_open.orders.back().id // 3
						% m_security.GetSymbol().GetSymbol().c_str() // 4
						% m_self.GetTradingSystem().GetTag().c_str() // 5
						% m_tradingSystem.GetMode(); // 6
				});
			m_tradingSystem.CancelOrder(m_open.orders.back().id);
			isCanceled = true;
		}
		if (m_close.HasActiveOrders()) {
			Assert(!m_close.orders.empty());
			Assert(m_close.orders.back().isActive);
			m_strategy.GetTradingLog().Write(
				"order\tpos=%1%\torder=%2%/%3%\tcancel-all\tclose-order"
					"\t%4%\t%5%\t%6%",
				[this](TradingRecord &record) {
					record
						% m_operationId // 1
						% m_close.orders.back().uuid // 2
						% m_close.orders.back().id // 3
						% m_security.GetSymbol().GetSymbol().c_str() // 4
						% m_self.GetTradingSystem().GetTag().c_str() // 5
						% m_tradingSystem.GetMode(); // 6
				});
			Assert(!isCanceled);
			m_tradingSystem.CancelOrder(m_close.orders.back().id);
			isCanceled = true;
		}
		return isCanceled;
	}

private:

	void CopyOrder(
			const std::string *orderTradeSystemId,
			bool isOpen,
			const OrderStatus &status,
			const TimeInForce *timeInForce = nullptr,
			const OrderParams *orderParams = nullptr) {

		Assert(!m_open.orders.empty());
		Assert(isOpen || !m_close.orders.empty());
		
		m_strategy.GetContext().InvokeDropCopy(
			[&](DropCopy &dropCopy) {

				const Order &order = isOpen
					?	m_open.orders.back()
					:	m_close.orders.back();

				pt::ptime orderTime;
				pt::ptime execTime;
				double bidPrice;
				Qty bidQty;
				double askPrice;
				Qty askQty;
				static_assert(numberOfOrderStatuses == 9, "List changed");
				switch (status) {
					case ORDER_STATUS_SENT:
						orderTime = m_strategy.GetContext().GetCurrentTime();
						bidPrice = m_security.GetBidPrice();
						bidQty = m_security.GetBidQty();
						askPrice = m_security.GetAskPrice();
						askQty = m_security.GetAskQty();
						break;
					case ORDER_STATUS_CANCELLED:
					case ORDER_STATUS_FILLED:
					case ORDER_STATUS_FILLED_PARTIALLY:
					case ORDER_STATUS_REJECTED:
					case ORDER_STATUS_INACTIVE:
					case ORDER_STATUS_ERROR:
						execTime = m_strategy.GetContext().GetCurrentTime();
						break;
				}

				dropCopy.CopyOrder(
					order.uuid,
					orderTradeSystemId,
					!orderTime.is_not_a_date_time() ? &orderTime : nullptr,
					!execTime.is_not_a_date_time() ? &execTime : nullptr,
					status,
					m_operationId,
					&m_subOperationId,
					m_security,
					m_tradingSystem,
					m_self.GetType() == TYPE_LONG
						?	(isOpen ? ORDER_SIDE_BUY : ORDER_SIDE_SELL)
						:	(!isOpen ? ORDER_SIDE_BUY : ORDER_SIDE_SELL),
					order.qty,
					!IsZero(order.price) ? &order.price : nullptr,
					timeInForce,
					m_self.GetCurrency(),
					orderParams && orderParams->minTradeQty
						?	&*orderParams->minTradeQty
						:	nullptr,
					order.executedQty,
					status == ORDER_STATUS_SENT ? &bidPrice : nullptr,
					status == ORDER_STATUS_SENT ? &bidQty : nullptr,
					status == ORDER_STATUS_SENT ? &askPrice : nullptr,
					status == ORDER_STATUS_SENT ? &askQty : nullptr);

			});

	}

	void CopyTrade(
			const std::string &tradingSystemId,
			double price,
			const Qty &qty,
			bool isOpen) {

		Assert(!m_open.orders.empty());
		Assert(isOpen || !m_close.orders.empty());

		m_strategy.GetContext().InvokeDropCopy(
			[&](DropCopy &dropCopy) {

				const Order &order = isOpen
					?	m_open.orders.back()
					:	m_close.orders.back();

				dropCopy.CopyTrade(
					m_strategy.GetContext().GetCurrentTime(),
					tradingSystemId,
					order.uuid,
					price,
					qty,
					m_security.GetBidPrice(),
					m_security.GetBidQty(),
					m_security.GetAskPrice(),
					m_security.GetAskQty());

			});

	}

};


//////////////////////////////////////////////////////////////////////////

Position::Position(
		Strategy &strategy,
		const uuids::uuid &operationId,
		int64_t subOperationId,
		TradingSystem &tradingSystem,
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &startPrice,
		const TimeMeasurement::Milestones &timeMeasurement) {
	Assert(!strategy.IsBlocked());
	m_pimpl = new Implementation(
		*this,
		tradingSystem,
		strategy,
		operationId,
		subOperationId,
		security,
		currency,
		qty,
		startPrice,
		timeMeasurement);
	//...//
}

#pragma warning(push)
#pragma warning(disable: 4702)
Position::Position() {
	AssertFail("Position::Position exists only for virtual inheritance");
	throw LogicError("Position::Position exists only for virtual inheritance");
}
#pragma warning(pop)

Position::~Position() {
	delete m_pimpl;
}

const uuids::uuid & Position::GetId() const {
	return m_pimpl->m_operationId;
}

bool Position::IsLong() const {
	static_assert(numberOfTypes == 2, "List changed.");
	Assert(
		GetType() == Position::TYPE_LONG
		|| GetType() == Position::TYPE_SHORT);
	return GetType() == Position::TYPE_LONG;
}

const ContractExpiration & Position::GetExpiration() const {
	if (!m_pimpl->m_expiration) {
		Assert(m_pimpl->m_expiration);
		boost::format error("Position %1% %2% does not have expiration");
		error % GetSecurity() % GetId();
		throw LogicError(error.str().c_str());
	}
	return *m_pimpl->m_expiration;
}

const Strategy & Position::GetStrategy() const noexcept {
	return const_cast<Position *>(this)->GetStrategy();
}

Strategy & Position::GetStrategy() noexcept {
	return m_pimpl->m_strategy;
}

const Security & Position::GetSecurity() const noexcept {
	return const_cast<Position *>(this)->GetSecurity();
}
Security & Position::GetSecurity() noexcept {
	return m_pimpl->m_security;
}

const TradingSystem & Position::GetTradingSystem() const {
	return const_cast<Position *>(this)->GetTradingSystem();
}

TradingSystem & Position::GetTradingSystem() {
	return m_pimpl->m_tradingSystem;
}

const Currency & Position::GetCurrency() const {
	return m_pimpl->m_currency;
}

const TimeMeasurement::Milestones & Position::GetTimeMeasurement() {
	return m_pimpl->m_timeMeasurement;
}

Position::CloseType Position::GetCloseType() const noexcept {
	if (m_pimpl->m_close.orders.empty()) {
		return CLOSE_TYPE_NONE;
	}
	return m_pimpl->m_close.orders.back().closeType;
}

bool Position::IsOpened() const noexcept {
	return !HasActiveOpenOrders() && GetOpenedQty() > 0;
}
bool Position::IsClosed() const noexcept {
	return
		!HasActiveOrders()
		&& GetOpenedQty() > 0
		&& GetActiveQty() == 0;
}

bool Position::IsStarted() const noexcept {
	return !m_pimpl->m_open.orders.empty();
}

bool Position::IsCompleted() const noexcept {
	return
		m_pimpl->m_isMarketAsCompleted
		|| (IsStarted() && !HasActiveOrders() && GetActiveQty() == 0);
}

void Position::MarkAsCompleted() {
	Assert(!m_pimpl->m_isMarketAsCompleted);
	m_pimpl->m_isMarketAsCompleted = true;
	m_pimpl->m_strategy.OnPositionMarkedAsCompleted(*this);
}

bool Position::IsError() const noexcept {
	return m_pimpl->m_isError;
}

bool Position::IsInactive() const noexcept {
	return m_pimpl->m_isInactive;
}

void Position::ResetInactive() {
	Assert(m_pimpl->m_isInactive);
	m_pimpl->m_isInactive = false;
}

bool Position::IsCanceled() const noexcept {
	return m_pimpl->m_cancelState != Implementation::CANCEL_STATE_NOT_CANCELED;
}

bool Position::HasActiveOrders() const noexcept {
	return HasActiveCloseOrders() || HasActiveOpenOrders();
}
bool Position::HasActiveOpenOrders() const noexcept {
	return m_pimpl->m_open.HasActiveOrders();
}
bool Position::HasActiveCloseOrders() const noexcept {
	return m_pimpl->m_close.HasActiveOrders();
}

void Position::UpdateOpening(
		const OrderId &orderId,
		const std::string &tradingSystemOrderId,
		const OrderStatus &orderStatus,
		const Qty &remainingQty,
		const TradingSystem::TradeInfo *trade) {
	m_pimpl->UpdateOpening(
		orderId,
		tradingSystemOrderId,
		orderStatus,
		remainingQty,
		trade);
}

void Position::UpdateClosing(
		const OrderId &orderId,
		const std::string &tradingSystemOrderId,
		const OrderStatus &orderStatus,
		const Qty &remainingQty,
		const TradingSystem::TradeInfo *trade) {
	m_pimpl->UpdateClosing(
		orderId,
		tradingSystemOrderId,
		orderStatus,
		remainingQty,
		trade);
}

const pt::ptime & Position::GetOpenTime() const {
	return m_pimpl->m_open.time;
}

Qty Position::GetActiveQty() const noexcept {
	AssertGe(GetOpenedQty(), GetClosedQty());
	return GetOpenedQty() - GetClosedQty();
}

double Position::GetActiveVolume() const {
	const auto &activeQty = GetActiveQty();
	if (activeQty == GetOpenedQty()) {
		// actual:
		return GetOpenedVolume();
	}
	if (!m_pimpl->m_open.qty) {
		return 0;
	}
	// approximate:
	return activeQty * (GetOpenedVolume() / m_pimpl->m_open.qty);
}

const Qty & Position::GetClosedQty() const noexcept {
	return m_pimpl->m_close.qty;
}

double Position::GetClosedVolume() const {
	return m_pimpl->m_security.DescalePrice(m_pimpl->m_close.volume);
}

const pt::ptime & Position::GetCloseTime() const {
	return m_pimpl->m_close.time;
}

const ScaledPrice & Position::GetLastTradePrice() const {
	if (m_pimpl->m_close.numberOfTrades) {
		return m_pimpl->m_close.lastTradePrice;
	} else if (m_pimpl->m_open.numberOfTrades) {
		return m_pimpl->m_open.lastTradePrice;
	} else {
		throw Exception("Position has no trades");
	}
}

double Position::GetRealizedPnlPercentage() const {
	const auto ratio = GetRealizedPnlRatio();
	const auto result = ratio > 1 || IsEqual(ratio, 1.0)
		? ratio - 1
		: -(1 - ratio);
	return result * 100;
}

double Position::GetPlannedPnl() const {
	return GetUnrealizedPnl() + GetRealizedPnl();
}

bool Position::IsProfit() const {
	const auto ratio = GetRealizedPnlRatio();
	return ratio > 1.0 && !IsEqual(ratio, 1.0);
}

size_t Position::GetNumberOfOpenTrades() const {
	return m_pimpl->m_open.numberOfTrades;
}

size_t Position::GetNumberOfCloseTrades() const {
	return m_pimpl->m_close.numberOfTrades;
}

Position::StateUpdateConnection Position::Subscribe(
		const StateUpdateSlot &slot)
		const {
	return StateUpdateConnection(m_pimpl->m_stateUpdateSignal.connect(slot));
}

const Qty & Position::GetPlanedQty() const {
	return m_pimpl->m_planedQty;
}

const ScaledPrice & Position::GetOpenStartPrice() const {
	return m_pimpl->m_open.startPrice;
}

const Qty & Position::GetOpenedQty() const noexcept {
	return m_pimpl->m_open.qty;
}
void Position::SetOpenedQty(const Qty &newQty) const noexcept {
	m_pimpl->m_open.qty = newQty;
	if (newQty > m_pimpl->m_planedQty) {
		m_pimpl->m_planedQty = newQty;
	}
}

ScaledPrice Position::GetOpenAvgPrice() const {
	if (!m_pimpl->m_open.qty) {
		throw Exception("Position has no open price");
	}
	return ScaledPrice(m_pimpl->m_open.volume / m_pimpl->m_open.qty);
}

const ScaledPrice & Position::GetLastOpenTradePrice() const {
	if (!m_pimpl->m_open.numberOfTrades) {
		throw Exception("Position has no open trades");
	}
	return m_pimpl->m_open.lastTradePrice;
}

double Position::GetOpenedVolume() const {
	return m_pimpl->m_security.DescalePrice(m_pimpl->m_open.volume);
}

const ScaledPrice & Position::GetCloseStartPrice() const {
	return m_pimpl->m_close.startPrice;
}

void Position::SetCloseStartPrice(const ScaledPrice &price) {
	m_pimpl->m_close.startPrice = price;
}

ScaledPrice Position::GetCloseAvgPrice() const {
	if (!m_pimpl->m_close.qty) {
		throw Exception("Position has no close price");
	}
	return ScaledPrice(m_pimpl->m_close.volume / m_pimpl->m_close.qty);
}

const ScaledPrice & Position::GetLastCloseTradePrice() const {
	if (!m_pimpl->m_close.numberOfTrades) {
		throw Exception("Position has no close trades");
	}
	return m_pimpl->m_close.lastTradePrice;
}

OrderId Position::OpenAtMarketPrice() {
	return OpenAtMarketPrice(defaultOrderParams);
}

OrderId Position::OpenAtMarketPrice(const OrderParams &params) {
	return m_pimpl->Open(
		[this](const Qty &qty, const OrderParams &params) {
			return DoOpenAtMarketPrice(qty, params);
		},
		TIME_IN_FORCE_DAY,
		params,
		false);
}

OrderId Position::Open(const ScaledPrice &price) {
	return Open(price, defaultOrderParams);
}

OrderId Position::Open(
		const ScaledPrice &price,
		const OrderParams &params) {
	return m_pimpl->Open(
		[this, &price](const Qty &qty, const OrderParams &params) {
			return DoOpen(qty, price, params);
		},
		TIME_IN_FORCE_DAY,
		params,
		price);
}

OrderId Position::OpenAtMarketPriceWithStopPrice(
		const ScaledPrice &stopPrice) {
	return OpenAtMarketPriceWithStopPrice(stopPrice, defaultOrderParams);
}

OrderId Position::OpenAtMarketPriceWithStopPrice(
		const ScaledPrice &stopPrice,
		const OrderParams &params) {
	return m_pimpl->Open(
		[this, stopPrice](const Qty &qty, const OrderParams &params) {
			return DoOpenAtMarketPriceWithStopPrice(qty, stopPrice, params);
		},
		TIME_IN_FORCE_DAY,
		params,
		false);
}

OrderId Position::OpenImmediatelyOrCancel(
		const ScaledPrice &price) {
	return OpenImmediatelyOrCancel(price, defaultOrderParams);
}

OrderId Position::OpenImmediatelyOrCancel(
		const ScaledPrice &price,
		const OrderParams &params) {
	return m_pimpl->Open(
		[this, price](const Qty &qty, const OrderParams &params) {
			return DoOpenImmediatelyOrCancel(qty, price, params);
		},
		TIME_IN_FORCE_IOC,
		params,
		true);
}

OrderId Position::OpenAtMarketPriceImmediatelyOrCancel() {
	return OpenAtMarketPriceImmediatelyOrCancel(defaultOrderParams);
}

OrderId Position::OpenAtMarketPriceImmediatelyOrCancel(
		const OrderParams &params) {
	return m_pimpl->Open(
		[this](const Qty &qty, const OrderParams &params) {
			return DoOpenAtMarketPriceImmediatelyOrCancel(qty, params);
		},
		TIME_IN_FORCE_IOC,
		params,
		false);
}

OrderId Position::CloseAtMarketPrice(
		const CloseType &closeType) {
	return CloseAtMarketPrice(closeType, defaultOrderParams);
}

OrderId Position::CloseAtMarketPrice(
		const CloseType &closeType,
		const OrderParams &params) {
	return m_pimpl->Close(
		closeType,
		[this](const Qty &qty, const OrderParams &params) {
			return DoCloseAtMarketPrice(qty, params);
		},
		TIME_IN_FORCE_DAY,
		0,
		GetActiveQty(),
		params);
}

OrderId Position::Close(const CloseType &closeType, const ScaledPrice &price) {
	return Close(closeType, price, GetActiveQty(), defaultOrderParams);
}

OrderId Position::Close(
		const CloseType &closeType,
		const ScaledPrice &price,
		const Qty &maxQty) {
	return Close(closeType, price, maxQty, defaultOrderParams);
}

OrderId Position::Close(
		const CloseType &closeType,
		const ScaledPrice &price,
		const OrderParams &params) {
	return Close(closeType, price, GetActiveQty(), params);
}

OrderId Position::Close(
		const CloseType &closeType,
		const ScaledPrice &price,
		const Qty &maxQty,
		const OrderParams &params) {
	return m_pimpl->Close(
		closeType,
		[this, &price](const Qty &qty, const OrderParams &params) {
			return DoClose(qty, price, params);
		},
		TIME_IN_FORCE_DAY,
		price,
		maxQty,
		params);
}

OrderId Position::CloseAtMarketPriceWithStopPrice(
		const CloseType &closeType,
		const ScaledPrice &stopPrice) {
	return CloseAtMarketPriceWithStopPrice(
		closeType,
		stopPrice,
		defaultOrderParams);
}

OrderId Position::CloseAtMarketPriceWithStopPrice(
		const CloseType &closeType,
		const ScaledPrice &stopPrice,
		const OrderParams &params) {
	return m_pimpl->Close(
		closeType,
		[this, stopPrice](const Qty &qty, const OrderParams &params) {
			return DoCloseAtMarketPriceWithStopPrice(qty, stopPrice, params);
		},
		TIME_IN_FORCE_DAY,
		0,
		GetActiveQty(),
		params);
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
		[this, price](const Qty &qty, const OrderParams &params) {
			return DoCloseImmediatelyOrCancel(qty, price, params);
		},
		TIME_IN_FORCE_IOC,
		price,
		GetActiveQty(),
		params);
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
		[this](const Qty &qty, const OrderParams &params) {
			return DoCloseAtMarketPriceImmediatelyOrCancel(qty, params);
		},
		TIME_IN_FORCE_IOC,
		0,
		GetActiveQty(),
		params);
}

bool Position::CancelAtMarketPrice(const CloseType &closeType) {
	return CancelAtMarketPrice(closeType, defaultOrderParams);
}

bool Position::CancelAtMarketPrice(
		const CloseType &closeType,
		const OrderParams &params) {
	return m_pimpl->CancelAtMarketPrice(
		closeType,
		[this](const Qty &qty, const OrderParams &params) {
			return DoCloseAtMarketPrice(qty, params);
		},
		TIME_IN_FORCE_DAY,
		params);
}

bool Position::CancelAllOrders() {
	return m_pimpl->CancelAllOrders();
}

TRDK_CORE_API std::ostream & trdk::operator <<(
		std::ostream &os,
		const Position::CloseType &closeType) {
	static_assert(
		Position::numberOfCloseTypes == 10,
		"Close type list changed.");
	switch (closeType) {
		default:
			AssertEq(Position::CLOSE_TYPE_NONE, closeType);
			os << "unknown";
			break;
		case Position::CLOSE_TYPE_NONE:
			os << "-";
			break;
		case Position::CLOSE_TYPE_TAKE_PROFIT:
			os << "take-profit";
			break;
		case Position::CLOSE_TYPE_STOP_LOSS:
			os << "stop-loss";
			break;
		case Position::CLOSE_TYPE_TIMEOUT:
			os << "timeout";
			break;
		case Position::CLOSE_TYPE_SCHEDULE:
			os << "schedule";
			break;
		case Position::CLOSE_TYPE_ROLLOVER:
			os << "rollover";
			break;
		case Position::CLOSE_TYPE_REQUEST:
			os << "request";
			break;
		case Position::CLOSE_TYPE_ENGINE_STOP:
			os << "engine stop";
			break;
		case Position::CLOSE_TYPE_OPEN_FAILED:
			os << "open failed";
			break;
		case Position::CLOSE_TYPE_SYSTEM_ERROR:
			os << "sys error";
			break;
	}
	return os;
}

//////////////////////////////////////////////////////////////////////////

LongPosition::LongPosition(
		Strategy &strategy,
		const uuids::uuid &operationId,
		int64_t subOperationId,
		TradingSystem &tradingSystem,
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &startPrice,
		const TimeMeasurement::Milestones &timeMeasurement)
	: Position(
		strategy,
		operationId,
		subOperationId,
		tradingSystem,
		security,
		currency,
		qty,
		startPrice,
		timeMeasurement) {
	GetStrategy().GetTradingLog().Write(
		"position\tnew\tlong\tpos=%1%\t%2%\t%3%.%4%"
			"\tprice=%5$.8f\t%6%\tqty=%7$.8f",
		[this](TradingRecord &record) {
			record
				% GetId() // 1
				% GetSecurity().GetSymbol().GetSymbol().c_str() // 2
				% GetTradingSystem().GetTag().c_str() // 3
				% GetTradingSystem().GetMode() // 4
				% GetSecurity().DescalePrice(GetOpenStartPrice()) // 5
				% GetCurrency() // 6
				% GetPlanedQty(); // 7 and last
		});
}

LongPosition::LongPosition() {
	GetStrategy().GetTradingLog().Write(
		"position\tnew\tlong\tpos=%1%\t%2%\t%3%.%4%"
			"\tprice=%5$.8f\t%6%\tqty=%7$.8f",
		[this](TradingRecord &record) {
			record
				% GetId() // 1
				% GetSecurity().GetSymbol().GetSymbol().c_str() // 2
				% GetTradingSystem().GetTag() // 3
				% GetTradingSystem().GetMode() // 4
				% GetSecurity().DescalePrice(GetOpenStartPrice()) // 5
				% GetCurrency() // 6
				% GetPlanedQty(); // 7 and last
		});
}

LongPosition::~LongPosition() {
	GetStrategy().GetTradingLog().Write(
		"position\tdel\tlong\tpos=%1%",
		[this](TradingRecord &record) {record % GetId();});
}

LongPosition::Type LongPosition::GetType() const {
	return TYPE_LONG;
}

namespace {
	const std::string longPositionTypeName = "long";
}
const std::string & LongPosition::GetTypeStr() const noexcept {
	return longPositionTypeName;
}

double LongPosition::GetRealizedPnl() const {
	const auto &activeQty = GetActiveQty();
	if (activeQty == 0) {
		return GetClosedVolume() - GetOpenedVolume();
	}
	const auto openedVolume
		= (GetOpenedQty() - activeQty) 
			* GetSecurity().DescalePrice(GetOpenAvgPrice());
	return GetClosedVolume() - openedVolume;
}

double LongPosition::GetRealizedPnlRatio() const {
	const auto &activeQty = GetActiveQty();
	if (activeQty == 0) {
		const auto openedVolume = GetOpenedVolume();
		return IsZero(openedVolume)
			?	0
			:	GetClosedVolume() / openedVolume;
	}
	const auto openPrice = GetOpenAvgPrice();
	if (!openPrice) {
		return 0;
	}
	const auto openedVolume
		= (GetOpenedQty() - activeQty) 
			* GetSecurity().DescalePrice(openPrice);
	return GetClosedVolume() / openedVolume;
}

double LongPosition::GetUnrealizedPnl() const {
	return (GetActiveQty() * GetSecurity().GetBidPrice()) - GetActiveVolume();
}

ScaledPrice LongPosition::GetMarketOpenPrice() const {
	return GetSecurity().GetAskPriceScaled();
}

ScaledPrice LongPosition::GetMarketClosePrice() const {
	return GetSecurity().GetBidPriceScaled();
}

ScaledPrice LongPosition::GetMarketOpenOppositePrice() const {
	return GetSecurity().GetBidPriceScaled();
}

ScaledPrice LongPosition::GetMarketCloseOppositePrice() const {
	return GetSecurity().GetAskPriceScaled();
}

OrderId LongPosition::DoOpenAtMarketPrice(
		const Qty &qty,
		const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().BuyAtMarketPrice(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId LongPosition::DoOpen(
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().Buy(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId LongPosition::DoOpenAtMarketPriceWithStopPrice(
		const Qty &qty,
		const ScaledPrice &stopPrice,
		const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().BuyAtMarketPriceWithStopPrice(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId LongPosition::DoOpenImmediatelyOrCancel(
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().BuyImmediatelyOrCancel(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId LongPosition::DoOpenAtMarketPriceImmediatelyOrCancel(
		const Qty &qty,
		const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().BuyAtMarketPriceImmediatelyOrCancel(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId LongPosition::DoCloseAtMarketPrice(
		const Qty &qty,
		const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().SellAtMarketPrice(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId LongPosition::DoClose(
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().Sell(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId LongPosition::DoCloseAtMarketPriceWithStopPrice(
		const Qty &qty,
		const ScaledPrice &stopPrice,
		const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().SellAtMarketPriceWithStopPrice(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId LongPosition::DoCloseImmediatelyOrCancel(
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	AssertLt(0, qty);
	return GetTradingSystem().SellImmediatelyOrCancel(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId LongPosition::DoCloseAtMarketPriceImmediatelyOrCancel(
		const Qty &qty,
		const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	AssertLt(0, qty);
	return GetTradingSystem().SellAtMarketPriceImmediatelyOrCancel(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

//////////////////////////////////////////////////////////////////////////

ShortPosition::ShortPosition(
		Strategy &strategy,
		const uuids::uuid &operationId,
		int64_t subOperationId,
		TradingSystem &tradingSystem,
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &startPrice,
		const TimeMeasurement::Milestones &timeMeasurement)
	: Position(
		strategy,
		operationId,
		subOperationId,
		tradingSystem,
		security,
		currency,
		qty,
		startPrice,
		timeMeasurement) {
	GetStrategy().GetTradingLog().Write(
		"position\tnew\tshort\tpos=%1%\t%2%\t%3%.%4%"
			"\tprice=%5$.8f\t%6%\tqty=%7$.8f",
		[this](TradingRecord &record) {
			record
				% GetId() // 1
				% GetSecurity().GetSymbol().GetSymbol().c_str() // 2
				% GetTradingSystem().GetTag().c_str() // 3
				% GetTradingSystem().GetMode() // 4
				% GetSecurity().DescalePrice(GetOpenStartPrice()) // 5
				% GetCurrency() // 6
				% GetPlanedQty(); // 7 and last
		});
}

ShortPosition::ShortPosition() {
	GetStrategy().GetTradingLog().Write(
		"position\tnew\tshort\tpos=%1%\t%2%\t%3%.%4%"
			"\tprice=%5$.8f\t%6%\tqty=%7$.8f",
		[this](TradingRecord &record) {
			record
				% GetId() // 1
				% GetSecurity().GetSymbol().GetSymbol().c_str() // 2
				% GetTradingSystem().GetTag().c_str() // 3
				% GetTradingSystem().GetMode() // 4
				% GetSecurity().DescalePrice(GetOpenStartPrice()) // 5
				% GetCurrency() // 6
				% GetPlanedQty(); // 7 and last
		});
}

ShortPosition::~ShortPosition() {
	GetStrategy().GetTradingLog().Write(
		"position\tdel\tshort\tpos=%1%",
		[this](TradingRecord &record) {record % GetId();});
}

ShortPosition::Type ShortPosition::GetType() const {
	return TYPE_SHORT;
}

namespace {
	const std::string shortTypeName = "short";
}
const std::string & ShortPosition::GetTypeStr() const noexcept {
	return shortTypeName;
}

double ShortPosition::GetRealizedPnl() const {
	if (GetActiveQty() == 0) {
		return GetOpenedVolume() - GetClosedVolume();
	}
	const auto openedVolume
		= (GetOpenedQty() - GetActiveQty())
			* GetSecurity().DescalePrice(GetOpenAvgPrice());
	return openedVolume - GetClosedVolume();
}
double ShortPosition::GetRealizedPnlRatio() const {
	const auto closedVolume = GetClosedVolume();
	if (IsZero(closedVolume)) {
		return 0;
	}
	if (GetActiveQty() == 0) {
		return GetOpenedVolume() / closedVolume;
	}
	const auto openedVolume
		= (GetOpenedQty() - GetActiveQty())
			* GetSecurity().DescalePrice(GetOpenAvgPrice());
	return openedVolume / closedVolume;
}
double ShortPosition::GetUnrealizedPnl() const {
	return GetActiveVolume() - (GetActiveQty() * GetSecurity().GetAskPrice());
}

ScaledPrice ShortPosition::GetMarketOpenPrice() const {
	return GetSecurity().GetBidPriceScaled();
}

ScaledPrice ShortPosition::GetMarketClosePrice() const {
	return GetSecurity().GetAskPriceScaled();
}

ScaledPrice ShortPosition::GetMarketOpenOppositePrice() const {
	return GetSecurity().GetAskPriceScaled();
}

ScaledPrice ShortPosition::GetMarketCloseOppositePrice() const {
	return GetSecurity().GetBidPriceScaled();
}

OrderId ShortPosition::DoOpenAtMarketPrice(
		const Qty &qty,
		const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().SellAtMarketPrice(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId ShortPosition::DoOpen(
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().Sell(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId ShortPosition::DoOpenAtMarketPriceWithStopPrice(
		const Qty &qty,
		const ScaledPrice &stopPrice,
		const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().SellAtMarketPriceWithStopPrice(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId ShortPosition::DoOpenImmediatelyOrCancel(
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().SellImmediatelyOrCancel(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId ShortPosition::DoOpenAtMarketPriceImmediatelyOrCancel(
		const Qty &qty,
		const OrderParams &params) {
	Assert(!IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().SellAtMarketPriceImmediatelyOrCancel(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId ShortPosition::DoCloseAtMarketPrice(
		const Qty &qty,
		const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().BuyAtMarketPrice(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId ShortPosition::DoClose(
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params) {
	return GetTradingSystem().Buy(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId ShortPosition::DoCloseAtMarketPriceWithStopPrice(
		const Qty &qty,
		const ScaledPrice &stopPrice,
		const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().BuyAtMarketPriceWithStopPrice(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId ShortPosition::DoCloseImmediatelyOrCancel(
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().BuyImmediatelyOrCancel(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId ShortPosition::DoCloseAtMarketPriceImmediatelyOrCancel(
		const Qty &qty,
		const OrderParams &params) {
	Assert(IsOpened());
	Assert(!IsClosed());
	return GetTradingSystem().BuyAtMarketPriceImmediatelyOrCancel(
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

//////////////////////////////////////////////////////////////////////////
