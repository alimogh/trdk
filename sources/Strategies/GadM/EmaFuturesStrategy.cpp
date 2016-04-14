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

		enum Direction {
			DIRECTION_UP,
			DIRECTION_LEVEL,
			DIRECTION_DOWN
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
			, m_security(nullptr)
			, m_fastEmaDirection(DIRECTION_LEVEL) {
			GetLog().Info(
				"Number of contracts: %1%. Passive order max. lifetime: %2%.",
				m_numberOfContracts,
				m_passiveOrderMaxLifetime);
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

			if (boost::istarts_with(maService->GetTag(), "fast")) {
				GetLog().Info(
					"Using EMA service \"%1%\" as fast EMA...",
					maService->GetTag());
				if (m_ema[FAST].HasData()) {
					throw Exception(
						"Strategy should have one one fast EMA service");
				}
				m_ema[FAST] = Ema(*maService);
			} else if (boost::istarts_with(maService->GetTag(), "slow")) {
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

		virtual void OnLevel1Update(Security &, const Milestones &) {
			//...//
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
					"Signal to %1%: slow=%2%, fast=%3%",
					[&](TradingRecord &record) {
						record
							% (signal == DIRECTION_UP ? "BUY" : "SELL")
							% m_ema[SLOW].GetValue()
							% m_ema[FAST].GetValue();
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
				position.CancelAllOrders();
				return;
			}

			position.SetIntention(INTENTION_CLOSE_PASSIVE);
			timeMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_2);

		}

		void CheckSlowOrderFilling(Position &position) {

			if (
					position.GetIntention() != INTENTION_OPEN_PASSIVE
					&& position.GetIntention() != INTENTION_CLOSE_PASSIVE) {
				return;
			}

			const auto orderExpirationTime
				= position.GetStartTime() + m_passiveOrderMaxLifetime;
			if (orderExpirationTime < GetContext().GetCurrentTime()) {
				return;
			}

			position.SetIntention(
				position.GetIntention() == INTENTION_OPEN_PASSIVE
					?	INTENTION_OPEN_AGGRESIVE
					:	INTENTION_CLOSE_AGGRESIVE);

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
