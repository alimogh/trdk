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

//////////////////////////////////////////////////////////////////////////

namespace {
	
	const OrderParams defaultOrderParams;

	class UuidGenerator : private boost::noncopyable {
	public:
		uuids::uuid operator ()() {
			const Lib::Concurrency::SpinScopedLock lock(m_mutex);
			return m_generator();
		}
	private:
		Lib::Concurrency::SpinMutex m_mutex;
		uuids::random_generator m_generator;
	} generateUuid;

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

	struct StaticData {
		uuids::uuid uuid;
		bool hasPrice;
		TimeInForce timeInForce;
	};

	struct DynamicData : public StaticData {

		boost::atomic<OrderId> orderId;
		Time time;
		struct Price {
			boost::atomic_intmax_t total;
			boost::atomic_size_t count;
			Price()
				: total(0)
				, count(0) {
				//...//
			}
		} price;
		boost::atomic<Qty::ValueType> qty;
		boost::atomic<ScaledPrice> comission;

		boost::atomic_bool hasOrder;

		DynamicData()
			: orderId(nOrderId)
			, qty(0)
			, comission(0)
			, hasOrder(false) {
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

	TradingSystem &m_tradingSystem;

	mutable Mutex m_mutex;

	mutable StateUpdateSignal m_stateUpdateSignal;

	Strategy &m_strategy;
	const uuids::uuid m_operationId;
	const int64_t m_subOperationId;
	bool m_isRegistered;
	Security &m_security;
	const Currency m_currency;

	boost::atomic<Qty::ValueType> m_planedQty;

	const ScaledPrice m_openStartPrice;
	DynamicData m_opened;
	boost::optional<ContractExpiration> m_expiration;

	ScaledPrice m_closeStartPrice;
	DynamicData m_closed;

	boost::atomic<CloseType> m_closeType;

	bool m_isMarketAsCompleted;

	boost::atomic_bool m_isError;
	boost::atomic_bool m_isInactive;

	boost::atomic<CancelState> m_cancelState;
	boost::function<void ()> m_cancelMethod;

	TimeMeasurement::Milestones m_timeMeasurement;

	boost::shared_ptr<Position> m_oppositePosition;

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
		: m_position(position)
		, m_tradingSystem(tradingSystem)
		, m_strategy(strategy)
		, m_operationId(operationId)
		, m_subOperationId(subOperationId)
		, m_isRegistered(false)
		, m_security(security)
		, m_currency(currency)
		, m_planedQty(qty)
		, m_openStartPrice(startPrice)
		, m_closeStartPrice(0)
		, m_closeType(CLOSE_TYPE_NONE)
		, m_isMarketAsCompleted(false)
		, m_isError(false)
		, m_isInactive(false)
		, m_cancelState(CANCEL_STATE_NOT_CANCELED)
		, m_timeMeasurement(timeMeasurement) {
		AssertLt(0, m_planedQty);
	}

public:

	void UpdateOpening(
			const OrderId &orderId,
			const std::string &tradingSystemOrderId,
			const OrderStatus &orderStatus,
			const Qty &remainingQty,
			const TradingSystem::TradeInfo *trade) {

		bool isCompleted = false;

		boost::shared_ptr<Position> updatedOppositePosition;

		{

			const WriteLock lock(m_mutex);
			std::unique_ptr<const WriteLock> oppositePositionLock;
			if (m_oppositePosition) {
				oppositePositionLock.reset(
					new WriteLock(m_oppositePosition->m_pimpl->m_mutex));
			}

			Assert(!m_position.IsOpened());
			Assert(!m_position.IsClosed());
			Assert(!m_position.IsCompleted());
			AssertNe(0, orderId);
			AssertNe(0, m_opened.orderId);
			AssertEq(0, m_closed.price.total);
			AssertEq(0, m_closed.price.count);
			Assert(m_opened.time.is_not_a_date_time());
			Assert(m_closed.time.is_not_a_date_time());
			AssertLe(m_opened.qty, m_planedQty);
			AssertEq(0, m_closed.qty);
			Assert(m_opened.hasOrder);
			Assert(!m_closed.hasOrder);

			if (m_opened.orderId != orderId) {
				ReportOrderIdReplace(true, tradingSystemOrderId);
				m_closed.orderId = orderId;
			}

			static_assert(
				numberOfOrderStatuses == 9,
				"List changed.");

			switch (orderStatus) {
				default:
					AssertFail("Unknown order status");
					return;
				case ORDER_STATUS_SENT:
				case ORDER_STATUS_REQUESTED_CANCEL:
					AssertFail("Status can be set only by this object.");
				case ORDER_STATUS_SUBMITTED:
					AssertEq(0, m_opened.qty);
					AssertEq(0, m_opened.price.total);
					AssertEq(0, m_opened.price.count);
					AssertEq(
						m_planedQty
							+	(m_oppositePosition
									?	m_oppositePosition->GetActiveQty()
									:	Qty(0)),
						remainingQty);
					Assert(!trade);
					AssertLt(0, remainingQty);
					return;
				case ORDER_STATUS_FILLED:
				case ORDER_STATUS_FILLED_PARTIALLY:
					if (!trade) {
						throw Exception("Filled order has no trade information");
					}
					{
						auto tradeQty = trade->qty;
						if (m_oppositePosition) {
							AssertGe(
								m_oppositePosition->GetActiveQty()
									+ m_position.GetPlanedQty(),
								tradeQty);
							auto filledForOpposite = tradeQty;
							if (filledForOpposite > m_oppositePosition->GetActiveQty()) {
								filledForOpposite
									= m_oppositePosition->GetActiveQty();
							}
							AssertGe(tradeQty, filledForOpposite);
							tradeQty -= filledForOpposite;
							m_oppositePosition->m_pimpl->m_closed.price.total
								+= trade->price;
							++m_oppositePosition->m_pimpl->m_closed.price.count;
							m_oppositePosition->m_pimpl->m_closed.qty
								= m_oppositePosition->m_pimpl->m_closed.qty
									+ filledForOpposite;
							AssertLt(
								0,
								m_oppositePosition->m_pimpl->m_closed.qty);
							m_oppositePosition->m_pimpl->ReportClosingUpdate(
								"filled",
								tradingSystemOrderId,
								orderStatus);
							updatedOppositePosition = m_oppositePosition;
						}
						if (tradeQty != 0) {
							AssertEq(
								m_opened.qty + tradeQty + remainingQty,
								m_planedQty);
							m_opened.price.total += trade->price;
							++m_opened.price.count;
							m_opened.qty = m_opened.qty + tradeQty;
							AssertLe(0, m_opened.qty);
							ReportOpeningUpdate(
								"filled",
								tradingSystemOrderId,
								orderStatus);
						} else {
							AssertNe(0, remainingQty);
						}
					}
					isCompleted = remainingQty == 0;
					CopyTrade(
						trade->id,
						m_security.DescalePrice(trade->price),
						trade->qty,
						true);
					break;
				case ORDER_STATUS_INACTIVE:
					isCompleted = true;
					if (m_oppositePosition) {
						m_oppositePosition->m_pimpl->ReportClosingUpdate(
							"inactive",
							tradingSystemOrderId,
							orderStatus);
						m_oppositePosition->m_pimpl->m_isInactive = true;
						updatedOppositePosition = m_oppositePosition;
					}
					ReportOpeningUpdate(
						"inactive",
						tradingSystemOrderId,
						orderStatus);
					m_isInactive = true;
					break;
				case ORDER_STATUS_ERROR:
					isCompleted = true;
					if (m_oppositePosition) {
						m_oppositePosition->m_pimpl->ReportClosingUpdate(
							"error",
							tradingSystemOrderId,
							orderStatus);
						m_oppositePosition->m_pimpl->m_isError = true;
						updatedOppositePosition = m_oppositePosition;
					}
					ReportOpeningUpdate(
						"error",
						tradingSystemOrderId,
						orderStatus);
					m_isError = true;
					break;
				case ORDER_STATUS_CANCELLED:
				case ORDER_STATUS_REJECTED:
					isCompleted = true;
					if (m_oppositePosition) {
						m_oppositePosition->m_pimpl->ReportClosingUpdate(
							TradingSystem::GetStringStatus(orderStatus),
							tradingSystemOrderId,
							orderStatus);
						updatedOppositePosition = m_oppositePosition;
					}
					ReportOpeningUpdate(
						TradingSystem::GetStringStatus(orderStatus),
						tradingSystemOrderId,
						orderStatus);
					break;
			}
	
			if (m_oppositePosition) {
				if (m_oppositePosition->GetActiveQty() == 0) {
					try {
						m_oppositePosition->m_pimpl->m_closed.time
							= m_security.GetContext().GetCurrentTime();
					} catch (...) {
						AssertFailNoException();
					}
					m_oppositePosition->m_pimpl->m_closed.hasOrder = false;
					updatedOppositePosition = m_oppositePosition;
					m_oppositePosition.reset();
				} else if (isCompleted) {
					AssertNe(0, m_oppositePosition->GetCloseStartPrice());
					try {
						m_oppositePosition->SetCloseStartPrice(0);
					} catch (...) {
						AssertFailNoException();
					}
					m_oppositePosition->m_pimpl->m_closed.hasOrder = false;
					updatedOppositePosition = m_oppositePosition;
				}
			}

			if (isCompleted) {

				AssertLe(m_opened.qty, m_planedQty);
				Assert(m_opened.time.is_not_a_date_time());

				if (m_position.GetOpenedQty() > 0) {
					try {
						m_opened.time
							= m_security.GetContext().GetCurrentTime();
					} catch (...) {
						AssertFailNoException();
					}
				}

				m_opened.hasOrder = false;
				if (CancelIfSet()) {
					return;
				}

			}

			CopyOrder(&tradingSystemOrderId, true, orderStatus);

		}
		
		if (updatedOppositePosition) {
			updatedOppositePosition->m_pimpl->SignalUpdate();
		}
		if (isCompleted) {
			SignalUpdate();
		}

	}

	void UpdateClosing(
			const OrderId &orderId,
			const std::string &tradingSystemOrderId,
			const OrderStatus &orderStatus,
			const Qty &remainingQty,
			const TradingSystem::TradeInfo *trade) {

		{

			const WriteLock lock(m_mutex);

			Assert(!m_oppositePosition);
			Assert(m_position.IsOpened());
			Assert(!m_position.IsClosed());
			Assert(!m_position.IsCompleted());
			Assert(!m_opened.time.is_not_a_date_time());
			Assert(m_closed.time.is_not_a_date_time());
			AssertLe(m_opened.qty, m_planedQty);
			AssertLe(m_closed.qty, m_opened.qty);
			AssertNe(orderId, 0);
			AssertNe(m_closed.orderId, 0);
			AssertNe(m_opened.orderId, orderId);
			Assert(!m_opened.hasOrder);
			Assert(m_closed.hasOrder);

			if (m_closed.orderId != orderId) {
				ReportOrderIdReplace(false, tradingSystemOrderId);
				m_closed.orderId = orderId;
			}

			static_assert(
				numberOfOrderStatuses == 9,
				"List changed.");
			switch (orderStatus) {
				default:
					AssertFail("Unknown order status");
					return;
				case ORDER_STATUS_SENT:
				case ORDER_STATUS_REQUESTED_CANCEL:
					AssertFail("Status can be set only by this object.");
				case ORDER_STATUS_SUBMITTED:
					AssertEq(m_closed.qty, 0);
					AssertEq(m_closed.price.total, 0);
					AssertEq(m_closed.price.count, 0);
					AssertLt(0, remainingQty);
					return;
				case ORDER_STATUS_FILLED:
				case ORDER_STATUS_FILLED_PARTIALLY:
					if (!trade) {
						throw Exception("Filled order has no trade information");
					}
					AssertEq(trade->qty + remainingQty, m_opened.qty);
					AssertLe(m_closed.qty + trade->qty, m_opened.qty);
					AssertLt(0, trade->price);
					m_closed.price.total += trade->price;
					++m_closed.price.count;
					m_closed.qty = m_closed.qty + trade->qty;
					AssertGt(m_closed.qty, 0);
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
					ReportClosingUpdate(
						"error",
						tradingSystemOrderId,
						orderStatus);
					m_isInactive = true;
					break;
				case ORDER_STATUS_ERROR:
					ReportClosingUpdate(
						"error",
						tradingSystemOrderId,
						orderStatus);
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
	
			AssertLe(m_opened.qty, m_planedQty);
			Assert(m_closed.time.is_not_a_date_time());

			if (m_position.GetActiveQty() == 0) {
				try {
					m_closed.time = m_security.GetContext().GetCurrentTime();
				} catch (...) {
					m_isError = true;
					AssertFailNoException();
				}
			}

			m_closed.hasOrder = false;

			CopyOrder(&tradingSystemOrderId, false, orderStatus);

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
		try {
			m_cancelMethod();
			return true;
		} catch (...) {
			m_isError = true;
			AssertFailNoException();
			return false;
		}
	}

	void ReportOpeningStart(const char *eventDesc) const throw() {
		m_strategy.GetTradingLog().Write(
			"order\tpos=%1%\torder=%2%\t%3%\t%4%\t%5%\t%6%.%7%"
				"\tprice=%8$.8f\t%9%\tqty=%10$.8f",
			[this, eventDesc](TradingRecord &record) {
				record
					% m_operationId // 1
					% m_opened.uuid // 2
					% eventDesc // 3
					% m_position.GetTypeStr() // 4
					% m_security.GetSymbol().GetSymbol() // 5
					% m_position.GetTradingSystem().GetTag() // 6
					% m_tradingSystem.GetMode() // 7
					% m_security.DescalePrice(m_openStartPrice) // 8
					% m_position.GetCurrency() // 9
					% m_position.GetPlanedQty(); // 10 and last
			});
	}

	void ReportOpeningUpdate(
			const char *eventDesc,
			const std::string &tsOrderId,
			const OrderStatus &orderStatus)
			const
			throw() {
		m_strategy.GetTradingLog().Write(
			"order\t%1%\tpos=%1%\torder=%2%/%3%\topen-%4%->%5%\t%6%\t%7%\t%8%.%9%"
				"\tprice=%10$.8f->%11$.8f\t%12%\tqty=%13$.8f->%14$.8f",
			[this, eventDesc, &tsOrderId, &orderStatus](
					TradingRecord &record) {
				record
					% m_operationId // 1
					% m_opened.uuid // 2
					% tsOrderId // 3
					% eventDesc // 4
					% m_tradingSystem.GetStringStatus(orderStatus) // 5
					% m_position.GetTypeStr() // 6
					% m_security.GetSymbol().GetSymbol() // 7
					% m_position.GetTradingSystem().GetTag() // 8
					% m_tradingSystem.GetMode() // 9
					% m_security.DescalePrice(m_position.GetOpenStartPrice()) // 10
					% m_security.DescalePrice(m_position.GetOpenPrice()) // 11
					% m_position.GetCurrency() // 12
					% m_position.GetPlanedQty() // 13
					% m_position.GetOpenedQty(); // 14 and last
					
			});
	}

	void ReportClosingStart(const char *eventDesc) const throw() {
		m_strategy.GetTradingLog().Write(
			"order\tpos=%1%\torder=%2%\tclose-%3%\t%4%\t%5%\t%6%.%7%"
				"\tprice=%8$.8f->%9$.8f\t%10%\tqty=%11$.8f",
			[this, eventDesc](TradingRecord &record) {
				record
					% m_operationId // 1
					% m_closed.uuid // 2
					% eventDesc // 3
					% m_position.GetTypeStr() // 4
					% m_security.GetSymbol().GetSymbol() // 5
					% m_position.GetTradingSystem().GetTag() // 6
					% m_tradingSystem.GetMode() // 7
					% m_security.DescalePrice(m_position.GetOpenPrice()) // 8
					% m_security.DescalePrice(m_position.GetCloseStartPrice()) // 9
					% m_position.GetCurrency() // 10
					% m_position.GetOpenedQty(); // 11 and last
			});
	}

	void ReportClosingUpdate(
			const char *eventDesc,
			const std::string &tsOrderId,
			const OrderStatus &orderStatus)
			const
			throw() {
		m_strategy.GetTradingLog().Write(
			"order\tpos=%1%\torder=%2%/%3%\tclose-%4%->%5%\t%6%\t%7%\t%8%.%9%"
				"\tprice=%10$.8f->%11$.8f\t%12%\tqty=%13$.8f->%14$.8f",
			[this, eventDesc, &tsOrderId, &orderStatus](TradingRecord &record) {
				record
					% m_operationId // 1
					% m_closed.uuid // 2
					% tsOrderId // 3
					% eventDesc // 4
					% m_tradingSystem.GetStringStatus(orderStatus) // 5
					% m_position.GetTypeStr() // 6
					% m_security.GetSymbol().GetSymbol() // 7
					% m_position.GetTradingSystem().GetTag() // 8
					% m_tradingSystem.GetMode() // 9
					% m_security.DescalePrice(m_position.GetCloseStartPrice()) // 10
					% m_security.DescalePrice(m_position.GetClosePrice()) // 11
					% m_position.GetCurrency() // 12
					% m_position.GetOpenedQty() // 13
					% m_position.GetClosedQty(); // 14 and last
			});
	}

	void ReportOrderIdReplace(
			bool isOpening,
			const std::string &tsOrderId)
			const
			throw() {
		m_strategy.GetTradingLog().Write(
			"order\tpos=%1%\torder=%2%/%3%\ttreplacing-%4%-order\t%5%",
			[&](TradingRecord &record) {
				record
					% m_operationId // 1
					% (isOpening ? m_opened.uuid : m_closed.uuid) // 2
					% tsOrderId // 3
					% (isOpening ? "open" : "close") // 4
					% (!isOpening ? m_opened.uuid : m_closed.uuid); // 5 and last
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

		m_opened.time = m_security.GetContext().GetCurrentTime();
		m_opened.qty = m_planedQty.load();
		m_opened.orderId = openOrderId;

		if (!m_isRegistered) {
			m_strategy.Register(m_position);
			m_isRegistered = true;
		}

		ReportOpeningUpdate("restored", std::string(), ORDER_STATUS_FILLED);

		SignalUpdate();
	
	}

	template<typename OpenImpl>
	OrderId Open(
			const OpenImpl &openImpl,
			const TimeInForce &timeInForce,
			const OrderParams &orderParams,
			bool hasPrice) {
		
		const WriteLock lock(m_mutex);
		std::unique_ptr<const WriteLock> oppositePositionLock;
		if (m_position.HasActiveOrders() || m_closed.orderId != nOrderId) {
			throw AlreadyStartedError();
		}
		
		Assert(!m_position.IsOpened());
		Assert(!m_position.IsError());
		Assert(!m_position.IsClosed());

		auto qtyToOpen = m_position.GetNotOpenedQty();
		const char *action = "open-pre";

		if (m_oppositePosition) {
			oppositePositionLock.reset(
				new WriteLock(m_oppositePosition->m_pimpl->m_mutex));
			Assert(m_oppositePosition->m_pimpl->m_isRegistered);
			Assert(m_oppositePosition->IsStarted());
			Assert(!m_oppositePosition->IsError());
			Assert(!m_oppositePosition->HasActiveOrders());
			AssertEq(0, m_oppositePosition->GetCloseStartPrice());
			if (!m_oppositePosition->IsOpened()) {
				throw NotOpenedError();
			} else if (
						m_oppositePosition->HasActiveCloseOrders()
						|| m_oppositePosition->IsClosed()) {
				throw AlreadyClosedError();
			}
			action = "close-open-pre";
			qtyToOpen += m_oppositePosition->GetActiveQty();
			m_oppositePosition->SetCloseStartPrice(m_openStartPrice);
		}

		if (!m_isRegistered) {
			m_strategy.Register(m_position);
			// supporting prev. logic (when was m_strategy = nullptr),
			// don't know why set flag in other place.
		}

		const StaticData staticData = {
			generateUuid(),
			hasPrice,
			timeInForce,
		};

		m_opened.uuid = staticData.uuid;
		ReportOpeningStart(action);

		try {
			OrderId orderId;
			if (
					!orderParams.expiration
					&& !m_security.GetSymbol().IsExplicit()) {
				m_expiration = m_security.GetExpiration();
				OrderParams additionalOrderParams(orderParams);
				additionalOrderParams.expiration = &*m_expiration;
				orderId = openImpl(qtyToOpen, additionalOrderParams);
			} else {
				orderId = openImpl(qtyToOpen, orderParams);
			}
			m_isRegistered = true;	// supporting prev. logic
									// (when was m_strategy = nullptr),
									// don't know why set flag only here.
			m_opened.StaticData::operator =(staticData);
			m_opened.hasOrder = true;
			m_opened.orderId = orderId;
			if (m_oppositePosition) {
				m_oppositePosition->m_pimpl->m_closeType = CLOSE_TYPE_NONE;
				m_oppositePosition->m_pimpl->m_closed.hasOrder = true;
				m_oppositePosition->m_pimpl->m_closed.orderId = orderId;
			}
			CopyOrder(
				nullptr, // order ID (from trading system)
				true,
				ORDER_STATUS_SENT,
				&timeInForce,
				&orderParams);
			return orderId;
		} catch (...) {
			if (m_isRegistered) {
				m_strategy.Unregister(m_position);
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
			throw;
		}
	
	}

	template<typename CloseImpl>
	OrderId CloseUnsafe(
			const CloseType &closeType,
			const CloseImpl &closeImpl,
			const TimeInForce &timeInForce,
			const OrderParams &orderParams,
			bool hasPrice,
			bool hasUuid) {
		
		if (!m_position.IsOpened()) {
			throw NotOpenedError();
		} else if (m_position.HasActiveCloseOrders() || m_position.IsClosed()) {
			throw AlreadyClosedError();
		}

		Assert(!m_oppositePosition);
		Assert(m_isRegistered);
		Assert(m_position.IsStarted());
		Assert(!m_position.IsError());
		Assert(!m_position.HasActiveOrders());
		Assert(!m_position.IsCompleted());

		if (!hasUuid) {
			m_closed.uuid = generateUuid();
		}
		ReportClosingStart("pre");


		OrderId orderId;
		if (m_expiration && !orderParams.expiration) {
			OrderParams additionalOrderParams(orderParams);
			additionalOrderParams.expiration = &*m_expiration;
			orderId = closeImpl(m_position.GetActiveQty(), additionalOrderParams);
		} else {
			orderId = closeImpl(m_position.GetActiveQty(), orderParams);
		}
		m_closeType = closeType;
		m_closed.hasPrice = hasPrice;
		m_closed.hasOrder = true;
		m_closed.orderId = orderId;

		CopyOrder(
			nullptr, // order ID (from trading system)
			false,
			ORDER_STATUS_SENT,
			&timeInForce,
			&orderParams);

		return orderId;

	}

	template<typename CloseImpl>
	OrderId Close(
			CloseType closeType,
			const CloseImpl &closeImpl,
			const TimeInForce &timeInForce,
			const OrderParams &orderParams,
			bool hasPrice) {
		const Implementation::WriteLock lock(m_mutex);
		return CloseUnsafe(
			closeType,
			closeImpl,
			timeInForce,
			orderParams,
			hasPrice,
			false);
	}

	template<typename CancelMethodImpl>
	bool CancelAtMarketPrice(
			CloseType closeType,
			const CancelMethodImpl &cancelMethodImpl,
			const TimeInForce &timeInForce,
			const OrderParams &orderParams,
			bool hasPrice) {
		
		const WriteLock lock(m_mutex);
		if (m_position.IsCanceled()) {
			Assert(!m_position.HasActiveOpenOrders());
			Assert(!m_position.HasActiveCloseOrders());
			return false;
		}

		m_closed.uuid = generateUuid();
		ReportClosingStart("cancel-pre");

		if (	m_position.IsClosed()
				|| (	!m_position.IsOpened()
						&& !m_position.HasActiveOpenOrders())) {
			return false;
		}

		if (CancelAllOrders()) {
			boost::function<void ()> delayedCancelMethod
				= [
						this,
						closeType,
						cancelMethodImpl,
						timeInForce,
						orderParams,
						hasPrice]() {
					if (!m_position.IsOpened() || m_position.IsClosed()) {
						SignalUpdate();
						return;
					}
					CloseUnsafe<CancelMethodImpl>(
						closeType,
						cancelMethodImpl,
						timeInForce,
						orderParams,
						hasPrice,
						true);
				};
			Assert(m_position.HasActiveOrders());
			delayedCancelMethod.swap(m_cancelMethod);
			AssertEq(int(CANCEL_STATE_NOT_CANCELED), int(m_cancelState));
			m_cancelState = CANCEL_STATE_SCHEDULED;
		} else {
			Assert(!m_position.HasActiveOrders());
			CloseUnsafe<CancelMethodImpl>(
				closeType,
				cancelMethodImpl,
				timeInForce,
				orderParams,
				hasPrice,
				true);
			AssertEq(int(CANCEL_STATE_NOT_CANCELED), int(m_cancelState));
			m_cancelState = CANCEL_STATE_CANCELED;
		}

		return true;
	
	}

	bool CancelAllOrders() {
		bool isCanceled = false;
		if (m_opened.hasOrder) {
			m_tradingSystem.CancelOrder(m_opened.orderId);
			isCanceled = true;
		}
		if (m_closed.hasOrder) {
			Assert(!isCanceled);
			m_tradingSystem.CancelOrder(m_closed.orderId);
			isCanceled = true;
		}
		return isCanceled;
	}

private:

	void CopyOrder(
			const std::string *orderId,
			bool isOpen,
			const OrderStatus &status,
			const TimeInForce *timeInForce = nullptr,
			const OrderParams *orderParams = nullptr) {
		
		m_strategy.GetContext().InvokeDropCopy(
			[&](DropCopy &dropCopy) {

				const DynamicData &directionData = isOpen
					?	m_opened
					:	m_closed;

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

				double price = .0;
				if (directionData.hasPrice) {
					price = m_security.DescalePrice(
						m_position.GetOpenStartPrice());
				}

				dropCopy.CopyOrder(
					directionData.uuid,
					orderId,
					!orderTime.is_not_a_date_time() ? &orderTime : nullptr,
					!execTime.is_not_a_date_time() ? &execTime : nullptr,
					status,
					m_operationId,
					&m_subOperationId,
					m_security,
					m_tradingSystem,
					m_position.GetType() == TYPE_LONG
						?	(isOpen ? ORDER_SIDE_BUY : ORDER_SIDE_SELL)
						:	(!isOpen ? ORDER_SIDE_BUY : ORDER_SIDE_SELL),
					m_position.GetPlanedQty(),
					directionData.hasPrice ? &price : nullptr,
					timeInForce,
					m_position.GetCurrency(),
					orderParams && orderParams->minTradeQty
						?	&*orderParams->minTradeQty
						:	nullptr,
					directionData.qty.load(),
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

		m_strategy.GetContext().InvokeDropCopy(
			[&](DropCopy &dropCopy) {

				const DynamicData &directionData = isOpen
					?	m_opened
					:	m_closed;

				dropCopy.CopyTrade(
					m_strategy.GetContext().GetCurrentTime(),
					tradingSystemId,
					directionData.uuid,
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

Position::Position(
		Strategy &strategy,
		const uuids::uuid &operationId,
		int64_t subOperationId,
		Position &oppositePosition,
		const Qty &qty,
		const ScaledPrice &startPrice,
		const TimeMeasurement::Milestones &timeMeasurement) {
	Assert(!strategy.IsBlocked());
	Assert(oppositePosition.IsOpened());
	Assert(!oppositePosition.HasActiveCloseOrders());
	Assert(!oppositePosition.IsClosed());
	Assert(oppositePosition.m_pimpl->m_isRegistered);
	Assert(oppositePosition.IsStarted());
	Assert(!oppositePosition.IsError());
	Assert(!oppositePosition.HasActiveOrders());
	Assert(!oppositePosition.IsCompleted());
	m_pimpl = new Implementation(
		*this,
		oppositePosition.m_pimpl->m_tradingSystem,
		strategy,
		operationId,
		subOperationId,
		oppositePosition.m_pimpl->m_security,
		oppositePosition.m_pimpl->m_currency,
		qty,
		startPrice,
		timeMeasurement);
	m_pimpl->m_oppositePosition = oppositePosition.shared_from_this();
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

const Strategy & Position::GetStrategy() const throw() {
	return const_cast<Position *>(this)->GetStrategy();
}

Strategy & Position::GetStrategy() throw() {
	return m_pimpl->m_strategy;
}

const Security & Position::GetSecurity() const throw() {
	return const_cast<Position *>(this)->GetSecurity();
}
Security & Position::GetSecurity() throw() {
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
	return
		m_pimpl->m_isMarketAsCompleted
		|| (IsStarted() && !HasActiveOrders() && GetActiveQty() == 0);
}

void Position::MarkAsCompleted() {
	Assert(!m_pimpl->m_isMarketAsCompleted);
	m_pimpl->m_isMarketAsCompleted = true;
	m_pimpl->m_strategy.OnPositionMarkedAsCompleted(*this);
}

bool Position::IsError() const throw() {
	return m_pimpl->m_isError ? true : false;
}

bool Position::IsInactive() const throw() {
	return m_pimpl->m_isInactive ? true : false;
}

void Position::ResetInactive() {
	Assert(m_pimpl->m_isInactive);
	m_pimpl->m_isInactive = false;
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
	const std::string takeProfit = "take profit";
	const std::string stopLoss = "stop loss";
	const std::string timeout = "timeout";
	const std::string schedule = "schedule";
	const std::string rollover = "rollover";
	const std::string request = "request";
	const std::string engineStop = "engine stop";
	const std::string openFailed = "open failed";
	const std::string systemError = "sys error";
} }

const std::string & Position::GetCloseTypeStr() const {
	using namespace CloseTypeStr;
	static_assert(numberOfCloseTypes == 10, "Close type list changed.");
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
		case CLOSE_TYPE_ROLLOVER:
			return rollover;
		case CLOSE_TYPE_REQUEST:
			return request;
		case CLOSE_TYPE_ENGINE_STOP:
			return engineStop;
		case CLOSE_TYPE_OPEN_FAILED:
			return openFailed;
		case CLOSE_TYPE_SYSTEM_ERROR:
			return systemError;
	}
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
	return m_pimpl->m_closed.qty.load();
}

double Position::GetClosedVolume() const {
	return GetSecurity().DescalePrice(GetClosePrice()) * GetClosedQty();
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
	return m_pimpl->m_planedQty.load();
}

const ScaledPrice & Position::GetOpenStartPrice() const {
	return m_pimpl->m_openStartPrice;
}

OrderId Position::GetOpenOrderId() const throw() {
	return m_pimpl->m_opened.orderId;
}
Qty Position::GetOpenedQty() const throw() {
	return m_pimpl->m_opened.qty.load();
}
void Position::SetOpenedQty(const Qty &newQty) const throw() {
	const Implementation::WriteLock lock(m_pimpl->m_mutex);
	m_pimpl->m_opened.qty = newQty;
	if (newQty > m_pimpl->m_planedQty) {
		m_pimpl->m_planedQty = newQty;
	}
}

ScaledPrice Position::GetOpenPrice() const {
	return m_pimpl->m_opened.price.count > 0
		?	ScaledPrice(m_pimpl->m_opened.price.total / m_pimpl->m_opened.price.count)
		:	0;
}

double Position::GetOpenedVolume() const {
	return GetSecurity().DescalePrice(GetOpenPrice()) * GetOpenedQty();
}

ScaledPrice Position::GetCloseStartPrice() const {
	return m_pimpl->m_closeStartPrice;
}

void Position::SetCloseStartPrice(const ScaledPrice &price) {
	m_pimpl->m_closeStartPrice = price;
}

ScaledPrice Position::GetClosePrice() const {
	return m_pimpl->m_closed.price.count > 0
		?	ScaledPrice(m_pimpl->m_closed.price.total / m_pimpl->m_closed.price.count)
		:	0;
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
		true);
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
		params,
		false);
}

OrderId Position::Close(const CloseType &closeType, const ScaledPrice &price) {
	return Close(closeType, price, defaultOrderParams);
}

OrderId Position::Close(
		const CloseType &closeType,
		const ScaledPrice &price,
		const OrderParams &params) {
	return m_pimpl->Close(
		closeType,
		[this, &price](const Qty &qty, const OrderParams &params) {
			return DoClose(qty, price, params);
		},
		TIME_IN_FORCE_DAY,
		params,
		true);
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
		params,
		false);
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
		params,
		true);
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
		params,
		false);
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
		params,
		false);
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
				% GetSecurity().GetSymbol().GetSymbol() // 2
				% GetTradingSystem().GetTag() // 3
				% GetTradingSystem().GetMode() // 4
				% GetSecurity().DescalePrice(GetOpenStartPrice()) // 5
				% GetCurrency() // 6
				% GetPlanedQty(); // 7 and last
		});
}

LongPosition::LongPosition(
		Strategy &strategy,
		const uuids::uuid &operationId,
		int64_t subOperationId,
		ShortPosition &oppositePosition,
		const Qty &qty,
		const ScaledPrice &startPrice,
		const TimeMeasurement::Milestones &timeMeasurement)
	: Position(
		strategy,
		operationId,
		subOperationId,
		oppositePosition,
		qty,
		startPrice,
		timeMeasurement) {
	GetStrategy().GetTradingLog().Write(
		"position\tcontinue\tlong\tpos=%1%->%2%\t%3%\t%4%.%5%"
			"\tprice=%6$.8f\t%7%\tqty=%8$.8f",
		[this, &oppositePosition](TradingRecord &record) {
			record
				% GetId() // 1
				% oppositePosition.GetId() // 2
				% GetSecurity().GetSymbol().GetSymbol() // 3
				% GetTradingSystem().GetTag() // 4
				% GetTradingSystem().GetMode() // 5
				% GetSecurity().DescalePrice(GetOpenStartPrice()) // 6
				% GetCurrency() // 7
				% GetPlanedQty(); // 8 and last
		});
}

LongPosition::LongPosition() {
	GetStrategy().GetTradingLog().Write(
		"position\tnew\tlong\tpos=%1%\t%2%\t%3%.%4%"
			"\tprice=%5$.8f\t%6%\tqty=%7$.8f",
		[this](TradingRecord &record) {
			record
				% GetId() // 1
				% GetSecurity().GetSymbol().GetSymbol() // 2
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
const std::string & LongPosition::GetTypeStr() const throw() {
	return longPositionTypeName;
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
				% GetSecurity().GetSymbol().GetSymbol() // 2
				% GetTradingSystem().GetTag() // 3
				% GetTradingSystem().GetMode() // 4
				% GetSecurity().DescalePrice(GetOpenStartPrice()) // 5
				% GetCurrency() // 6
				% GetPlanedQty(); // 7 and last
		});
}

ShortPosition::ShortPosition(
		Strategy &strategy,
		const uuids::uuid &operationId,
		int64_t subOperationId,
		LongPosition &oppositePosition,
		const Qty &qty,
		const ScaledPrice &startPrice,
		const TimeMeasurement::Milestones &timeMeasurement)
	: Position(
		strategy,
		operationId,
		subOperationId,
		oppositePosition,
		qty,
		startPrice,
		timeMeasurement) {
	GetStrategy().GetTradingLog().Write(
		"position\tcontinue\tshort\tpos=%1%->%2%\t%3%\t%4%.%5%"
			"\tprice=%6$.8f\t%7%\tqty=%8$.8f",
		[this, &oppositePosition](TradingRecord &record) {
			record
				% GetId() // 1
				% oppositePosition.GetId() // 2
				% GetSecurity().GetSymbol().GetSymbol() // 3
				% GetTradingSystem().GetTag() // 4
				% GetTradingSystem().GetMode() // 5
				% GetSecurity().DescalePrice(GetOpenStartPrice()) // 6
				% GetCurrency() // 7
				% GetPlanedQty(); // 8 and last
		});
}

ShortPosition::ShortPosition() {
	GetStrategy().GetTradingLog().Write(
		"position\tnew\tshort\tpos=%1%\t%2%\t%3%.%4%"
			"\tprice=%5$.8f\t%6%\tqty=%7$.8f",
		[this](TradingRecord &record) {
			record
				% GetId() // 1
				% GetSecurity().GetSymbol().GetSymbol() // 2
				% GetTradingSystem().GetTag() // 3
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
const std::string & ShortPosition::GetTypeStr() const throw() {
	return shortTypeName;
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
