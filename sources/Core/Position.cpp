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

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;

//////////////////////////////////////////////////////////////////////////

namespace {
	
	const char *logTag = "Position";

	const OrderParams defaultOrderParams;

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

	boost::atomic<PositionId> theNextPositionId(0);
	
}

class Position::Implementation : private boost::noncopyable {

public:

	typedef PostionConcurrencyPolicy::Mutex Mutex;
	typedef PostionConcurrencyPolicy::ReadLock ReadLock;
	typedef PostionConcurrencyPolicy::WriteLock WriteLock;

	typedef boost::signals2::signal<StateUpdateSlotSignature>
		StateUpdateSignal;

	struct StaticData {
		boost::uuids::uuid uuid;
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

	const PositionId m_id;

	Position &m_position;

	TradeSystem &m_tradeSystem;

	mutable Mutex m_mutex;

	mutable StateUpdateSignal m_stateUpdateSignal;

	Strategy &m_strategy;
	bool m_isRegistered;
	Security &m_security;
	const Currency m_currency;

	boost::atomic<Qty> m_planedQty;

	const ScaledPrice m_openStartPrice;
	DynamicData m_opened;

	ScaledPrice m_closeStartPrice;
	DynamicData m_closed;

	boost::atomic<CloseType> m_closeType;

	volatile long m_isStarted;

	bool m_isMarketAsCompleted;

	boost::atomic_bool m_isError;
	boost::atomic_bool m_isInactive;

	boost::atomic<CancelState> m_cancelState;
	boost::function<void ()> m_cancelMethod;

	TimeMeasurement::Milestones m_timeMeasurement;

	boost::shared_ptr<Position> m_oppositePosition;

	explicit Implementation(
			Position &position,
			TradeSystem &tradeSystem,
			Strategy &strategy,
			Security &security,
			const Currency &currency,
			const Qty &qty,
			const ScaledPrice &startPrice,
			const TimeMeasurement::Milestones &timeMeasurement)
		: m_id(theNextPositionId++),
		m_position(position),
		m_tradeSystem(tradeSystem),
		m_strategy(strategy),
		m_isRegistered(false),
		m_security(security),
		m_currency(currency),
		m_planedQty(qty),
		m_openStartPrice(startPrice),
		m_closeStartPrice(0),
		m_closeType(CLOSE_TYPE_NONE),
		m_isStarted(false),
		m_isMarketAsCompleted(false),
		m_isError(false),
		m_isInactive(false),
		m_cancelState(CANCEL_STATE_NOT_CANCELED),
		m_timeMeasurement(timeMeasurement) {
		AssertLt(0, m_planedQty);
	}

public:

	void UpdateOpening(
			const OrderId &orderId,
			const std::string &tradeSystemOrderId,
			const TradeSystem::OrderStatus &orderStatus,
			const Qty &remainingQty,
			const TradeSystem::TradeInfo *trade) {

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
				ReportOrderIdReplace("open", m_closed.orderId, orderId);
				m_closed.orderId = orderId;
			}

			static_assert(
				TradeSystem::numberOfOrderStatuses == 7,
				"List changed.");

			switch (orderStatus) {
				default:
					AssertFail("Unknown order status");
					return;
				case TradeSystem::ORDER_STATUS_SENT:
				case TradeSystem::ORDER_STATUS_REQUESTED_CANCEL:
					AssertFail("Status can be set only by this object.");
				case TradeSystem::ORDER_STATUS_SUBMITTED:
					AssertEq(0, m_opened.qty);
					AssertEq(0, m_opened.price.total);
					AssertEq(0, m_opened.price.count);
					AssertEq(
						m_planedQty
							+	(m_oppositePosition
									?	m_oppositePosition->GetActiveQty()
									:	0),
						remainingQty);
					Assert(!trade);
					AssertLt(0, remainingQty);
					return;
				case TradeSystem::ORDER_STATUS_FILLED:
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
								+= filledForOpposite;
							AssertLt(0, m_oppositePosition->m_pimpl->m_closed.qty);
							m_oppositePosition->m_pimpl->ReportClosingUpdate(
								"filled",
								orderStatus);
							updatedOppositePosition = m_oppositePosition;
						}
						if (tradeQty != 0) {
							AssertEq(
								m_opened.qty + tradeQty + remainingQty,
								m_planedQty);
							m_opened.price.total += trade->price;
							++m_opened.price.count;
							m_opened.qty += tradeQty;
							AssertLe(0, m_opened.qty);
							ReportOpeningUpdate("filled", orderStatus);
						} else {
							AssertNe(0, remainingQty);
						}
					}
					isCompleted = remainingQty == 0;
					CopyTrade(
						trade->id,
						m_security.DescalePrice(trade->price),
						trade->qty,
						tradeSystemOrderId,
						true);
					break;
				case TradeSystem::ORDER_STATUS_INACTIVE:
					isCompleted = true;
					if (m_oppositePosition) {
						m_oppositePosition->m_pimpl->ReportClosingUpdate(
							"inactive",
							orderStatus);
						m_oppositePosition->m_pimpl->m_isInactive = true;
						updatedOppositePosition = m_oppositePosition;
					}
					ReportOpeningUpdate("inactive", orderStatus);
					m_isInactive = true;
					break;
				case TradeSystem::ORDER_STATUS_ERROR:
					isCompleted = true;
					if (m_oppositePosition) {
						m_oppositePosition->m_pimpl->ReportClosingUpdate(
							"error",
							orderStatus);
						m_oppositePosition->m_pimpl->m_isError = true;
						updatedOppositePosition = m_oppositePosition;
					}
					ReportOpeningUpdate("error", orderStatus);
					m_isError = true;
					break;
				case TradeSystem::ORDER_STATUS_CANCELLED:
					isCompleted = true;
					if (m_oppositePosition) {
						m_oppositePosition->m_pimpl->ReportClosingUpdate(
							"canceled",
							orderStatus);
						updatedOppositePosition = m_oppositePosition;
					}
					ReportOpeningUpdate("canceled", orderStatus);
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

			CopyOrder(
				&tradeSystemOrderId,
				trade ? &trade->id : nullptr,
				true,
				orderStatus);

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
			const std::string &tradeSystemOrderId,
			const TradeSystem::OrderStatus &orderStatus,
			const Qty &remainingQty,
			const TradeSystem::TradeInfo *trade) {

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
				ReportOrderIdReplace("close", m_closed.orderId, orderId);
				m_closed.orderId = orderId;
			}

			switch (orderStatus) {
				default:
					AssertFail("Unknown order status");
					return;
				case TradeSystem::ORDER_STATUS_SENT:
				case TradeSystem::ORDER_STATUS_REQUESTED_CANCEL:
					AssertFail("Status can be set only by this object.");
				case TradeSystem::ORDER_STATUS_SUBMITTED:
					AssertEq(m_closed.qty, 0);
					AssertEq(m_closed.price.total, 0);
					AssertEq(m_closed.price.count, 0);
					AssertLt(0, remainingQty);
					return;
				case TradeSystem::ORDER_STATUS_FILLED:
					if (!trade) {
						throw Exception("Filled order has no trade information");
					}
					AssertEq(trade->qty + remainingQty, m_opened.qty);
					AssertLe(Qty(m_closed.qty) + trade->qty, m_opened.qty);
					AssertLt(0, trade->price);
					m_closed.price.total += trade->price;
					++m_closed.price.count;
					m_closed.qty += trade->qty;
					AssertGt(m_closed.qty, 0);
					ReportClosingUpdate("filled", orderStatus);
					CopyTrade(
						trade->id,
						m_security.DescalePrice(trade->price),
						trade->qty,
						tradeSystemOrderId,
						false);
					if (remainingQty != 0) {
						return;
					}
					break;
				case TradeSystem::ORDER_STATUS_INACTIVE:
					ReportClosingUpdate("error", orderStatus);
					m_isInactive = true;
					break;
				case TradeSystem::ORDER_STATUS_ERROR:
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
					m_closed.time = m_security.GetContext().GetCurrentTime();
				} catch (...) {
					m_isError = true;
					AssertFailNoException();
				}
			}

			m_closed.hasOrder = false;

			CopyOrder(
				&tradeSystemOrderId,
				trade ? &trade->id : nullptr,
				false,
				orderStatus);

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

	void ReportOpeningUpdate(
				const char *eventDesc,
				const TradeSystem::OrderStatus &orderStatus)
			const
			throw() {
		m_security.GetContext().GetTradingLog().Write(
			logTag,
			"%1%\t%2%.%21%\t%3%\t%4%\topen-%5%\t%6%"
				"\tprice=%7%->%8%\t%9%\tqty=%10%->%11%"
				"\tbid=%12%/%13%\task=%14%/%15%\tadjusted=%16%"
				"\torder-id=%17%"
				"\torder-status=%18%\thas-orders=%19%\tis-error=%20%",
			[this, eventDesc, &orderStatus](TradingRecord &record) {
				record
					% m_id
					% m_position.GetTradeSystem().GetTag()
					% m_security
					% m_position.GetTypeStr()
					% eventDesc
					% m_strategy.GetTag()
					% m_security.DescalePrice(m_position.GetOpenStartPrice())
					% m_security.DescalePrice(m_position.GetOpenPrice())
					% m_position.GetCurrency()
					% m_position.GetPlanedQty()
					% m_position.GetOpenedQty()
					% m_security.GetBidPrice()
					% m_security.GetBidQty()
					% m_security.GetAskPrice()
					% m_security.GetAskQty()
					% (m_security.IsBookAdjusted() ? "yes" : "no")
					% m_position.GetOpenOrderId()
					% m_tradeSystem.GetStringStatus(orderStatus)
					% m_position.HasActiveOpenOrders()
					% (m_isError ? true : false)
					% m_tradeSystem.GetMode();
			});
	}

	void ReportClosingStart(const char *eventDesc) const throw() {
		m_security.GetContext().GetTradingLog().Write(
			logTag,
			"%13%\t%11%.%14%\t%1%\t%2%\tclose-%12%\t%3%\tqty=%4%->%5%"
				" order-id=%6%->%7% has-orders=%8%/%9% is-error=%10%",
			[this, eventDesc](TradingRecord &record) {
				record
					% m_security
					% m_position.GetTypeStr()
					% m_strategy.GetTag()
					% m_position.GetOpenedQty()
					% m_position.GetClosedQty()
					% m_position.GetOpenOrderId()
					% m_position.GetCloseOrderId()
					% m_position.HasActiveOpenOrders()
					% m_position.HasActiveCloseOrders()
					% (m_isError ? true : false)
					% m_position.GetTradeSystem().GetTag()
					% eventDesc
					% m_id
					% m_position.GetTradeSystem().GetMode();
			});
	}

	void ReportClosingUpdate(
				const char *eventDesc,
				const TradeSystem::OrderStatus &orderStatus)
			const
			throw() {
		m_security.GetContext().GetTradingLog().Write(
			logTag,
			"%14%\t%11%.%16%\t%1%\t%2%\tclose-%3%\t%4%"
				"\tqty=%5%->%6% price=%15%->%7% order-id=%8%->%9%"
				" order-status=%10% has-orders=%12%/%13%",
			[this, eventDesc, &orderStatus](TradingRecord &record) {
				record
					% m_position.GetSecurity().GetSymbol()
					% m_position.GetTypeStr()
					% eventDesc
					% m_strategy.GetTag()
					% m_position.GetOpenedQty()
					% m_position.GetClosedQty()
					% m_security.DescalePrice(m_position.GetClosePrice())
					% m_position.GetOpenOrderId()
					% m_position.GetCloseOrderId()
					% m_tradeSystem.GetStringStatus(orderStatus)
					% m_position.GetTradeSystem().GetTag()
					% m_position.HasActiveOpenOrders()
					% m_position.HasActiveCloseOrders()
					% m_id
					% m_security.DescalePrice(m_position.GetCloseStartPrice())
					% m_tradeSystem.GetMode();
			});
	}

	void ReportOrderIdReplace(
				bool isOpening,
				const OrderId &prevOrdeId,
				const OrderId &newOrdeId)
			const
			throw() {
		m_security.GetContext().GetTradingLog().Write(
			logTag,
			"%1%\t%2%.%16%\t%3%\t%4%\treplacing-%5%-order\t%6%\t%7%"
				"\tqty=%8%->%9% price=%10%->%11%"
				" bid=%12%/%13% ask=%14%/%15%",
			[&](TradingRecord &record) {
				record
					% m_id
					% m_position.GetTradeSystem().GetTag()
					% m_security.GetSymbol()
					% m_position.GetTypeStr()
					% (isOpening ? "open" : "close")
					% prevOrdeId
					% newOrdeId
					% m_position.GetOpenedQty()
					% m_position.GetClosedQty();
				if (isOpening) {
					record.Format(
						m_security.DescalePrice(m_position.GetOpenStartPrice()),
						m_security.DescalePrice(m_position.GetOpenPrice()));
				} else {
					record.Format(
						m_security.DescalePrice(
								m_position.GetCloseStartPrice()),
						m_security.DescalePrice(m_position.GetClosePrice()));
				}
				record
					% m_security.GetBidPrice()
					% m_security.GetBidQty()
					% m_security.GetAskPrice()
					% m_security.GetAskQty()
					% m_tradeSystem.GetMode();
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

		ReportOpeningUpdate("restored", TradeSystem::ORDER_STATUS_FILLED);

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
		if (m_position.IsStarted()) {
			throw AlreadyStartedError();
		}
		
		Assert(!m_position.IsOpened());
		Assert(!m_position.IsError());
		Assert(!m_position.IsClosed());
		Assert(!m_position.IsCompleted());
		Assert(!m_position.HasActiveOrders());

		auto qtyToOpen = m_position.GetNotOpenedQty();
		const char *action = "open-pre";

		if (m_oppositePosition) {
			oppositePositionLock.reset(
				new WriteLock(m_oppositePosition->m_pimpl->m_mutex));
			Assert(m_oppositePosition->m_pimpl->m_isRegistered);
			Assert(m_oppositePosition->IsStarted());
			Assert(!m_oppositePosition->IsError());
			Assert(!m_oppositePosition->HasActiveOrders());
			Assert(!m_oppositePosition->IsCompleted());
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
			boost::uuids::random_generator()(),
			hasPrice,
			timeInForce,
		};

		m_security.GetContext().GetTradingLog().Write(
			logTag,
			"%1%\t%2%.%14%\t%3%\t%4%\t%5%\t%6%"
				"\tstart-price=%7%\t%8%"
				"\tqty=%9%\tbid=%10%/%11%\task=%12%/%13% uuid=%15%",
			[&](TradingRecord &record) {
				record
					% m_id
					% m_position.GetTradeSystem().GetTag()
					% m_security
					% m_position.GetTypeStr()
					% action
					% m_strategy.GetTag()
					% m_security.DescalePrice(m_openStartPrice)
					% m_position.GetCurrency()
					% m_position.GetPlanedQty()
					% m_security.GetBidPrice()
					% m_security.GetBidQty()
					% m_security.GetAskPrice()
					% m_security.GetAskQty()
					% m_position.GetTradeSystem().GetMode()
					% staticData.uuid;
			});

		try {
			const auto orderId = openImpl(qtyToOpen);
			m_isRegistered = true;	// supporting prev. logic
									// (when was m_strategy = nullptr),
									// don't know why set flag only here.
			m_opened.StaticData::operator =(staticData);
			m_opened.hasOrder = true;
			AssertEq(nOrderId, m_opened.orderId);
			m_opened.orderId = orderId;
			Assert(!m_isStarted);
			m_isStarted = true;
			if (m_oppositePosition) {
				m_oppositePosition->m_pimpl->m_closeType = CLOSE_TYPE_NONE;
				m_oppositePosition->m_pimpl->m_closed.hasOrder = true;
				m_oppositePosition->m_pimpl->m_closed.orderId = orderId;
			}
			CopyOrder(
				nullptr, // order ID (from trade system)
				nullptr, // trade ID
				true,
				TradeSystem::ORDER_STATUS_SENT,
				&timeInForce,
				&orderParams);
			return orderId;
		} catch (...) {
			if (m_isRegistered) {
				m_strategy.Unregister(m_position);
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
			bool hasPrice) {
		
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

		const auto orderId = closeImpl(m_position.GetActiveQty());
		m_closeType = closeType;
		m_closed.uuid = boost::uuids::random_generator()();
		m_closed.hasPrice = hasPrice;
		m_closed.hasOrder = true;
		m_closed.orderId = orderId;

		CopyOrder(
			nullptr, // order ID (from trade system)
			nullptr, // trade ID
			false,
			TradeSystem::ORDER_STATUS_SENT,
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
		ReportClosingStart("pre");
		const Implementation::WriteLock lock(m_mutex);
		return CloseUnsafe(
			closeType,
			closeImpl,
			timeInForce,
			orderParams,
			hasPrice);
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
						hasPrice);
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
				hasPrice);
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

private:

	void CopyOrder(
			const std::string *orderId,
			const std::string *tradeId,
			bool isOpen,
			const TradeSystem::OrderStatus &status,
			const TimeInForce *timeInForce = nullptr,
			const OrderParams *orderParams = nullptr) {
		
		DropCopy *const dropCopy = m_strategy.GetContext().GetDropCopy();
		if (!dropCopy) {
			return;
		}

		const DynamicData &directionData = isOpen
			?	m_opened
			:	m_closed;

		pt::ptime orderTime;
		pt::ptime execTime;
		double bidPrice;
		Qty bidQty;
		double askPrice;
		Qty askQty;
		static_assert(TradeSystem::numberOfOrderStatuses == 7, "List changed");
		switch (status) {
			case TradeSystem::ORDER_STATUS_SENT:
				orderTime = m_strategy.GetContext().GetCurrentTime();
				bidPrice = m_security.GetBidPrice();
				bidQty = m_security.GetBidQty();
				askPrice = m_security.GetAskPrice();
				askQty = m_security.GetAskQty();
				break;
			case TradeSystem::ORDER_STATUS_CANCELLED:
			case TradeSystem::ORDER_STATUS_FILLED:
			case TradeSystem::ORDER_STATUS_INACTIVE:
			case TradeSystem::ORDER_STATUS_ERROR:
				execTime = m_strategy.GetContext().GetCurrentTime();
				break;
		}

		double price = .0;
		if (directionData.hasPrice) {
			price = m_security.DescalePrice(m_position.GetOpenStartPrice());
		}

		double avgTradePrice = .0;
		if (directionData.price.count != 0) {
			avgTradePrice
				= double(directionData.price.total) / directionData.price.count;
			avgTradePrice = m_security.DescalePrice(avgTradePrice);
		}

		dropCopy->CopyOrder(
			directionData.uuid,
			!orderTime.is_not_a_date_time() ? &orderTime : nullptr,
			orderId,
			m_strategy,
			m_security,
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
			nullptr /* user */,
			status,
			!execTime.is_not_a_date_time() ? &execTime : nullptr,
			tradeId,
			directionData.price.count,
			avgTradePrice,
			directionData.qty,
			nullptr /* counterAmount */,
			status == TradeSystem::ORDER_STATUS_SENT ? &bidPrice : nullptr,
			status == TradeSystem::ORDER_STATUS_SENT ? &bidQty : nullptr,
			status == TradeSystem::ORDER_STATUS_SENT ? &askPrice : nullptr,
			status == TradeSystem::ORDER_STATUS_SENT ? &askQty : nullptr);

	}

	void CopyTrade(
			const std::string &id,
			double tradePrice,
			const Qty &tradeQty,
			const std::string &orderId,
			bool isOpen) {

		DropCopy *const dropCopy = m_strategy.GetContext().GetDropCopy();
		if (!dropCopy) {
			return;
		}

		const DynamicData &directionData = isOpen
			?	m_opened
			:	m_closed;

		dropCopy->CopyTrade(
			m_strategy.GetContext().GetCurrentTime(),
			id,
			m_strategy,
			false,
			tradePrice,
			tradeQty,
			.0 /* double counterAmount*/,
			directionData.uuid,
			orderId,
			m_security,
			m_position.GetType() == TYPE_LONG
				?	(isOpen ? ORDER_SIDE_BUY : ORDER_SIDE_SELL)
				:	(!isOpen ? ORDER_SIDE_BUY : ORDER_SIDE_SELL),
			m_position.GetPlanedQty(),
			m_security.DescalePrice(m_position.GetOpenStartPrice()),
			directionData.timeInForce,
			m_currency,
			0,
			"<UNKNOWN>",
			m_security.GetBidPrice(),
			m_security.GetBidQty(),
			m_security.GetAskPrice(),
			m_security.GetAskQty());

	}

};


//////////////////////////////////////////////////////////////////////////

Position::Position(
		Strategy &strategy,
		TradeSystem &tradeSystem,
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &startPrice,
		const TimeMeasurement::Milestones &timeMeasurement) {
	Assert(!strategy.IsBlocked());
	m_pimpl = new Implementation(
		*this,
		tradeSystem,
		strategy,
		security,
		currency,
		qty,
		startPrice,
		timeMeasurement);
}


Position::Position(
		Strategy &strategy,
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
		oppositePosition.m_pimpl->m_tradeSystem,
		strategy,
		oppositePosition.m_pimpl->m_security,
		oppositePosition.m_pimpl->m_currency,
		qty,
		startPrice,
		timeMeasurement);
	m_pimpl->m_oppositePosition = oppositePosition.shared_from_this();
}

Position::~Position() {
	delete m_pimpl;
}

const PositionId & Position::GetId() const {
	return m_pimpl->m_id;
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

const TradeSystem & Position::GetTradeSystem() const {
	return const_cast<Position *>(this)->GetTradeSystem();
}

TradeSystem & Position::GetTradeSystem() {
	return m_pimpl->m_tradeSystem;
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
	const std::string takeProfit = "t/p";
	const std::string stopLoss = "s/l";
	const std::string timeout = "timeout";
	const std::string schedule = "schedule";
	const std::string request = "request";
	const std::string engineStop = "engine stop";
	const std::string openFailed = "open failed";
	const std::string systemError = "sys error";
} }

const std::string & Position::GetCloseTypeStr() const {
	using namespace CloseTypeStr;
	static_assert(numberOfCloseTypes == 9, "Close type list changed.");
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
		const std::string &tradeSystemOrderId,
		const TradeSystem::OrderStatus &orderStatus,
		const Qty &remainingQty,
		const TradeSystem::TradeInfo *trade) {
	m_pimpl->UpdateOpening(
		orderId,
		tradeSystemOrderId,
		orderStatus,
		remainingQty,
		trade);
}

void Position::UpdateClosing(
		const OrderId &orderId,
		const std::string &tradeSystemOrderId,
		const TradeSystem::OrderStatus &orderStatus,
		const Qty &remainingQty,
		const TradeSystem::TradeInfo *trade) {
	m_pimpl->UpdateClosing(
		orderId,
		tradeSystemOrderId,
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

const ScaledPrice & Position::GetOpenStartPrice() const {
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
	if (m_pimpl->m_opened.qty > m_pimpl->m_planedQty) {
		m_pimpl->m_planedQty = m_pimpl->m_opened.qty.load();
	}
}

ScaledPrice Position::GetOpenPrice() const {
	return m_pimpl->m_opened.price.count > 0
		?	m_pimpl->m_opened.price.total / m_pimpl->m_opened.price.count
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
		?	m_pimpl->m_closed.price.total / m_pimpl->m_closed.price.count
		:	0;
}

OrderId Position::OpenAtMarketPrice() {
	return OpenAtMarketPrice(defaultOrderParams);
}

OrderId Position::OpenAtMarketPrice(const OrderParams &params) {
	return m_pimpl->Open(
		[&](const Qty &qty) -> OrderId {
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
		[&](const Qty &qty) -> OrderId {
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
		[&](const Qty &qty) -> OrderId {
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
		[&](const Qty &qty) -> OrderId {
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
		[&](const Qty &qty) {
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
		[&](const Qty &qty) -> OrderId {
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
		[&](const Qty &qty) -> OrderId {
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
		[&](const Qty &qty) -> OrderId {
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
		[&](const Qty &qty) {
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
		[&](const Qty &qty) {
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
		[&](const Qty &qty) -> OrderId {
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
		TradeSystem &tradeSystem,
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &startPrice,
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

LongPosition::LongPosition(
		Strategy &strategy,
		ShortPosition &oppositePosition,
		const Qty &qty,
		const ScaledPrice &startPrice,
		const TimeMeasurement::Milestones &timeMeasurement)
	: Position(
		strategy,
		oppositePosition,
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

OrderId LongPosition::DoOpenAtMarketPrice(
		const Qty &qty,
		const OrderParams &params) {
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId LongPosition::DoCloseAtMarketPrice(
		const Qty &qty,
		const OrderParams &params) {
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

//////////////////////////////////////////////////////////////////////////

ShortPosition::ShortPosition(
		Strategy &strategy,
		TradeSystem &tradeSystem,
		Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &startPrice,
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

ShortPosition::ShortPosition(
		Strategy &strategy,
		LongPosition &oppositePosition,
		const Qty &qty,
		const ScaledPrice &startPrice,
		const TimeMeasurement::Milestones &timeMeasurement)
	: Position(
		strategy,
		oppositePosition,
		qty,
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

OrderId ShortPosition::DoOpenAtMarketPrice(
		const Qty &qty,
		const OrderParams &params) {
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId ShortPosition::DoCloseAtMarketPrice(
		const Qty &qty,
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

OrderId ShortPosition::DoClose(
		const Qty &qty,
		const ScaledPrice &price,
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
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
			_5),
		GetStrategy().GetRiskControlScope(),
		GetTimeMeasurement());
}

//////////////////////////////////////////////////////////////////////////
