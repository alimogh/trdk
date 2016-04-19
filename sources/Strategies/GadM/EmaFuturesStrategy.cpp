/**************************************************************************
 *   Created: 2016/04/05 08:28:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "EmaFuturesStrategyPosition.hpp"
#include "Services/MovingAverageService.hpp"
#include "Core/Strategy.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Services;

namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Strategies { namespace GadM {
namespace EmaFuturesStrategy {

	////////////////////////////////////////////////////////////////////////////////

	enum Direction {
		DIRECTION_UP,
		DIRECTION_LEVEL,
		DIRECTION_DOWN
	};
	
	const char * ConvertToPch(const Direction &source) {
		switch (source) {
			case DIRECTION_UP:
				return "UP";
				break;
			case DIRECTION_LEVEL:
				return "LEVEL";
				break;
			case DIRECTION_DOWN:
				return "DOWN";
				break;
			default:
				AssertEq(DIRECTION_UP, source);
				return "<UNKNOWN>";
		}
	}

	////////////////////////////////////////////////////////////////////////////////

	class Strategy : public trdk::Strategy {

	public:

		typedef trdk::Strategy Base;

	private:
	
		class Ema {
		public:
			Ema()
				: m_value(0)
				, m_service(nullptr) {
				//...//
			}
			explicit Ema(const MovingAverageService &service)
				: m_value(0)
				, m_service(&service) {
				//...//
			}
			operator bool() const {
				return HasData();
			}
			bool HasSource() const {
				return m_service ? true : false;
			}
			bool HasData() const {
				return !IsZero(m_value);
			}
			bool CheckSource(const Service &service) {
				Assert(HasSource());
				if (m_service != &service) {
					return false;
				}
				m_value = m_service->GetLastPoint().value;
				return true;
			}
			double GetValue() const {
				Assert(HasSource());
				Assert(HasData());
				return m_value;
			}
		private:
			double m_value;
			const MovingAverageService *m_service;
		};

		enum EmaType {
			FAST = 0,
			SLOW = 1,
			numberOfEmaTypes
		};

	public:

		explicit Strategy(
				Context &context,
				const std::string &tag,
				const IniSectionRef &conf)
			: Base(context, "EmaFutures", tag, conf)
			, m_numberOfContracts(
				conf.ReadTypedKey<uint32_t>("number_of_contracts"))
			, m_passiveOrderMaxLifetime(
				pt::seconds(
					conf.ReadTypedKey<unsigned int>(
						"passive_order_max_lifetime_sec")))
			, m_orderPriceDelta(conf.ReadTypedKey<double>("order_price_delta"))
			, m_security(nullptr)
			, m_fastEmaDirection(DIRECTION_LEVEL) {
			GetLog().Info(
				"Number of contracts: %1%."
					" Passive order max. lifetime: %2%."
					" Order price delta: %3%.",
				m_numberOfContracts,
				m_passiveOrderMaxLifetime,
				m_orderPriceDelta);
		}
		
		virtual ~Strategy() {
			//...//
		}

	public:

		virtual pt::ptime OnSecurityStart(Security &security) {
			if (!m_security) {
				m_security = &security;
				GetLog().Info("Using \"%1%\" to trade...", *m_security);
			} else if (m_security != &security) {
				throw Exception(
					"Strategy can not work with more than one security");
			}
			return Base::OnSecurityStart(security);
		}

		virtual void OnServiceStart(const Service &service) {
			
			const auto *const maService
				= dynamic_cast<const MovingAverageService *>(&service);
			if (!maService) {
				throw Exception(
					"Strategy should be configured"
						" to work with MovingAverageService");
			}

			if (boost::iends_with(maService->GetTag(), "fastema")) {
				GetLog().Info(
					"Using EMA service \"%1%\" as fast EMA...",
					maService->GetTag());
				if (m_ema[FAST].HasData()) {
					throw Exception(
						"Strategy should have one one fast EMA service");
				}
				m_ema[FAST] = Ema(*maService);
			} else if (boost::iends_with(maService->GetTag(), "slowema")) {
				GetLog().Info(
					"Using EMA service \"%1%\" as slow EMA...",
					maService->GetTag());
				if (m_ema[SLOW].HasData()) {
					throw Exception(
						"Strategy should have one one slow EMA service");
				}
				m_ema[SLOW] = Ema(*maService);
			} else {
				GetLog().Error(
					"Failed to resolve EMA service \"%1%\".",
					maService->GetTag());
				throw Exception("Failed to resolve EMA service");
			}

			//! @todo Find another solution, without const_cast:
			OnSecurityStart(
				const_cast<Security &>(*maService->GetSecurities().GetBegin()));

		}

	protected:

		virtual void OnLevel1Update(Security &security, const Milestones &) {
			
			Assert(m_security == &security);
			UseUnused(security);
			
			if (!GetPositions().GetSize()) {
				return;
			}

			CheckSlowOrderFilling(
				dynamic_cast<Position &>(*GetPositions().GetBegin()));
		
		}
		
		virtual void OnPositionUpdate(trdk::Position &position) {
			Position &startegyPosition = dynamic_cast<Position &>(position);
			CheckSlowOrderFilling(startegyPosition);
			startegyPosition.Sync();
		}

		virtual void OnServiceDataUpdate(
				const Service &service,
				const Milestones &timeMeasurement) {
			
			m_ema[FAST].CheckSource(service)
				|| m_ema[SLOW].CheckSource(service);
			if (!IsDataStarted()) {
				return;
			}
			
			const Direction &signal = UpdateDirection();
			if (signal != DIRECTION_LEVEL) {
				GetTradingLog().Write(
					"signal\t%1%\tslow-ema=%2%\tfast-ema=%3%\t%4%\tbid=%5%\task=%6%",
					[&](TradingRecord &record) {
						record
							% (signal == DIRECTION_UP ? "BUY" : "SELL")
							% m_ema[SLOW].GetValue()
							% m_ema[FAST].GetValue()
							% ConvertToPch(m_fastEmaDirection)
							% m_security->GetBidPrice()
							% m_security->GetAskPrice();
					});
			}

			GetPositions().GetSize() > 0
				?	CheckPositionCloseSignal(signal, timeMeasurement)
				:	CheckPositionOpenSignal(signal, timeMeasurement);

		}

		virtual void OnPostionsCloseRequest() {
			throw MethodDoesNotImplementedError(
				"EmaFuturesStrategy::OnPostionsCloseRequest"
					" is not implemented");
		}

	private:

		bool IsDataStarted() {
			if (m_ema[FAST] && m_ema[SLOW]) {
				// Data started.
				Assert(m_ema[FAST].HasSource());
				Assert(m_ema[SLOW].HasSource());
				Assert(m_security);
				return true;
			}
			if (!m_ema[FAST].HasSource() || !m_ema[SLOW].HasSource()) {
				throw Exception(
					"Strategy doesn't have one or more EMA sources");
			}
			// Data not collected yet.
			return false;
		}

		void CheckPositionOpenSignal(
				const Direction &signal,
				const Milestones &timeMeasurement) {

			Assert(m_security);
			AssertEq(0, GetPositions().GetSize());

			boost::shared_ptr<Position> position;
			switch (signal) {
				case DIRECTION_LEVEL:
					timeMeasurement.Measure(SM_STRATEGY_WITHOUT_DECISION_1);
					return;
				case DIRECTION_UP:
					position = CreatePosition<LongPosition>(timeMeasurement);
					break;
				case DIRECTION_DOWN:
					position = CreatePosition<ShortPosition>(timeMeasurement);
					break;
				default:
					throw LogicError("Internal error: Unknown direction");
			}
			Assert(position);

			position->Sync();
			timeMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_1);
			Assert(position->HasActiveOpenOrders());

		}

		void CheckPositionCloseSignal(
				const Direction &signal,
				const Milestones &timeMeasurement) {

			Assert(m_security);
			AssertEq(1, GetPositions().GetSize());

			Position &position
				= dynamic_cast<Position &>(*GetPositions().GetBegin());
			if (position.GetIntention() > INTENTION_HOLD) {
				return;
			}

			switch (signal) {
				case  DIRECTION_LEVEL:
					return;
				case DIRECTION_UP:
					if (position.GetType() == Position::TYPE_LONG) {
						return;
					}
					break;
				case DIRECTION_DOWN:
					if (position.GetType() == Position::TYPE_SHORT) {
						return;
					}
					break;
			}

			if (!position.GetActiveQty()) {
				Assert(position.HasActiveOrders());
				Assert(position.HasActiveOpenOrders());
				GetTradingLog().Write(
					"canceling slow empty order by close signal",
					[&](TradingRecord &) {});
				position.SetIntention(INTENTION_DONOT_OPEN);
				return;
			}

			position.SetIntention(INTENTION_CLOSE_PASSIVE);
			timeMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_2);

		}

		void CheckSlowOrderFilling(Position &position) {

			static_assert(numberOfIntentions == 6, "List changed.");
			switch (position.GetIntention()) {
				
				case INTENTION_HOLD:
				case INTENTION_DONOT_OPEN:
					return;
				
				case INTENTION_OPEN_PASSIVE:
				case INTENTION_CLOSE_PASSIVE:
					{
						const auto orderExpirationTime
							= position.GetStartTime() + m_passiveOrderMaxLifetime;
						const auto &now = GetContext().GetCurrentTime();
						if (orderExpirationTime > now) {
							return;
						}
						GetTradingLog().Write(
							"slow-order\ttime\tstart=%1%\tend=%2%\tnow=%3%\t%4%",
							[&](TradingRecord &record) {
								record
									%	position.GetStartTime()
									%	orderExpirationTime
									%	now
									%	(position.GetIntention() == INTENTION_OPEN_PASSIVE
										?	"passive"
										:	"aggressive");
							});
						position.SetIntention(
							position.GetIntention() == INTENTION_OPEN_PASSIVE
								?	INTENTION_OPEN_AGGRESIVE
								:	INTENTION_CLOSE_AGGRESIVE);
						break;
					}

			}

			if (!position.IsPriceAllowed(m_orderPriceDelta)) {
				GetTradingLog().Write(
					"slow-order\tprice\tstart=%1%\tend=%2%\tnow=%3%\t%4%",
					[&](TradingRecord &record) {
						Assert(position.HasActiveOrders());
						//! @todo:
						if (position.HasActiveOpenOrders()) {
							record
								%	position.GetOpenStartPrice()
								%	(position.GetOpenStartPrice()
									+ m_orderPriceDelta)
								%	position.GetMarketOpenPrice();
						} else {
							record
								%	position.GetCloseStartPrice()
								%	(position.GetCloseStartPrice()
									+ m_orderPriceDelta)
								%	position.GetMarketClosePrice();
						}
						%	(position.GetIntention() == INTENTION_OPEN_PASSIVE
							?	"passive"
							:	"aggressive");
					});
				//! @todo
				position.SetIntention(
					position.GetIntention() == INTENTION_OPEN_PASSIVE
						?	INTENTION_OPEN_AGGRESIVE
						:	INTENTION_CLOSE_AGGRESIVE);
			}

		}

		Direction UpdateDirection() {
			
			auto fastEmaDirection
				= IsEqual(m_ema[FAST].GetValue(), m_ema[SLOW].GetValue())
					?	DIRECTION_LEVEL
					:	m_ema[FAST].GetValue() > m_ema[SLOW].GetValue()
						?	DIRECTION_UP
						:	DIRECTION_DOWN;

			if (m_fastEmaDirection == fastEmaDirection) {
				return DIRECTION_LEVEL;
			}

			std::swap(fastEmaDirection, m_fastEmaDirection);
			GetTradingLog().Write(
				"fast-ema\t%1%->%2%\tslow-ema=%3%\tfast-ema=%4%",
				[&](TradingRecord &record) {
					record
						% ConvertToPch(fastEmaDirection)
						% ConvertToPch(m_fastEmaDirection)
						% m_ema[SLOW]
						% m_ema[FAST];
				});

			switch (fastEmaDirection) {
				case  DIRECTION_LEVEL:
					// Intersection was at previous update.
					return DIRECTION_LEVEL;
				default:
					switch (m_fastEmaDirection) {
						case DIRECTION_UP:
						case DIRECTION_DOWN:
							return m_fastEmaDirection;
							break;
						case DIRECTION_LEVEL:
							return fastEmaDirection == DIRECTION_DOWN
								?	DIRECTION_UP
								:	DIRECTION_DOWN;
							break;
						default:
							throw LogicError(
								"Internal error: Unknown direction");
					}
			}

		}

		template<typename Position>
		boost::shared_ptr<Position> CreatePosition(
				const Milestones &timeMeasurement) {
			Assert(m_security);
			return boost::make_shared<Position>(
				*this,
				m_generateUuid(),
				GetTradeSystem(m_security->GetSource().GetIndex()),
				*m_security,
				m_numberOfContracts,
				timeMeasurement);
		}

	private:

		const Qty m_numberOfContracts;
		const pt::time_duration m_passiveOrderMaxLifetime;
		const double  m_orderPriceDelta;

		Security *m_security;

		boost::array<Ema, numberOfEmaTypes> m_ema;
		Direction m_fastEmaDirection;

		boost::uuids::random_generator m_generateUuid;

	};

} } } }

////////////////////////////////////////////////////////////////////////////////

TRDK_STRATEGY_GADM_API boost::shared_ptr<Strategy> CreateEmaFuturesStrategy(
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf) {
	using namespace trdk::Strategies::GadM;
	return boost::make_shared<EmaFuturesStrategy::Strategy>(context, tag, conf);
}

////////////////////////////////////////////////////////////////////////////////
