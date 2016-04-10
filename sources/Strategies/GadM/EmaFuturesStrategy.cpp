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
#include "Services/MovingAverageService.hpp"
#include "Core/Strategy.hpp"
#include "Core/MarketDataSource.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Services;

namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

namespace {

	template<typename Position>
	struct PositionTrait {
		//...//
	};
	template<>
	struct PositionTrait<LongPosition> {
		/** Submit a limit order, on the same side of the spread the system
		  * want to trade, joining the current level. So if we are buying,
		  * submit a buy at the current best bid level. In the jargon this is
		  * a passive behavior, waiting for the market to come to us.
		  */
		static ScaledPrice GetCurrentOpenAggressivePrice(
				const Security &security) {
			return security.GetAskPriceScaled();
		}
		static ScaledPrice GetCurrentOpenPassivePrice(
				const Security &security) {
			return security.GetBidPriceScaled();
		}
		static ScaledPrice GetCurrentCloseAggressivePrice(
				const Security &security) {
			return security.GetBidPriceScaled();
		}
		static ScaledPrice GetCurrentClosePassivePrice(
				const Security &security) {
			return security.GetAskPriceScaled();
		}
	};
	template<>
	struct PositionTrait<ShortPosition> {
		/** Submit a limit order, on the same side of the spread the system
		  * want to trade, joining the current level. So if we are buying,
		  * submit a buy at the current best bid level. In the jargon this is
		  * a passive behavior, waiting for the market to come to us.
		  */
		static ScaledPrice GetCurrentOpenAggressivePrice(
				const Security &security) {
			return security.GetBidPriceScaled();
		}
		static ScaledPrice GetCurrentOpenPassivePrice(
				const Security &security) {
			return security.GetAskPriceScaled();
		}
		static ScaledPrice GetCurrentCloseAggressivePrice(
				const Security &security) {
			return security.GetAskPriceScaled();
		}
		static ScaledPrice GetCurrentClosePassivePrice(
				const Security &security) {
			return security.GetBidPriceScaled();
		}
	};

}

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Strategies { namespace GadM {

	class EmaFuturesStrategy : public Strategy {

	public:

		typedef Strategy Base;

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

		explicit EmaFuturesStrategy(
				Context &context,
				const std::string &tag,
				const IniSectionRef &conf)
			: Base(context, "EmaFutures", tag, conf)
			, m_numberOfContracts(
				conf.ReadTypedKey<uint32_t>("number_of_contracts"))
			, m_security(nullptr)
			, m_fastEmaDirection(DIRECTION_LEVEL) {
			//.../
		}
		
		virtual ~EmaFuturesStrategy() {
			//...//
		}

	public:

		virtual pt::ptime OnSecurityStart(Security &security) {
			if (!m_security) {
				m_security = &security;
				GetLog().Info("Using \"%1%\" to trade...", m_security);
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

		}

	protected:

		virtual void OnServiceDataUpdate(
				const Service &service,
				const Milestones &timeMeasurement) {
			if (!IsDataStarted()) {
				return;
			}
			UpdateEma(service);
			GetPositions().GetSize() > 0
				?	CheckPositionCloseSignal(timeMeasurement)
				:	CheckPositionOpenSignal(timeMeasurement);
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

		void UpdateEma(const Service &service) {
			Assert(m_ema[FAST].HasSource());
			Assert(m_ema[SLOW].HasSource());
			Verify(
				m_ema[FAST].CheckSource(service)
				|| m_ema[SLOW].CheckSource(service));
		}

		void CheckPositionOpenSignal(const Milestones &timeMeasurement) {

			Assert(m_security);
			AssertEq(0, GetPositions().GetSize());

			boost::shared_ptr<Position> position;
			switch (UpdateDirection()) {
				case DIRECTION_LEVEL:
					timeMeasurement.Measure(SM_STRATEGY_WITHOUT_DECISION_1);
					return;
				case DIRECTION_UP:
					position = CreateOrder<LongPosition>(timeMeasurement);
					break;
				case DIRECTION_DOWN:
					position = CreateOrder<ShortPosition>(timeMeasurement);
					break;
				default:
					throw LogicError("Internal error: Unknown direction");
			}
			Assert(position);

			timeMeasurement.Measure(SM_STRATEGY_EXECUTION_START_1);
			position->Open(position->GetOpenStartPrice());
			timeMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_1);

		}

		void CheckPositionCloseSignal(const Milestones &timeMeasurement) {

			Assert(m_security);
			AssertEq(1, GetPositions().GetSize());

			const auto &direction = UpdateDirection();

			Position &position = *GetPositions().GetBegin();
			if (position.IsCompleted()) {
				return;
			}

			switch (direction) {
				case  DIRECTION_LEVEL:
					CheckSlowOrderExecution(timeMeasurement);
					return;
				case DIRECTION_UP:
					if (position.GetType() == Position::TYPE_LONG) {
						timeMeasurement.Measure(SM_STRATEGY_WITHOUT_DECISION_2);
						return;
					}
					break;
				case DIRECTION_DOWN:
					if (position.GetType() == Position::TYPE_SHORT) {
						timeMeasurement.Measure(SM_STRATEGY_WITHOUT_DECISION_2);
						return;
					}
					break;
			}

			timeMeasurement.Measure(SM_STRATEGY_EXECUTION_START_2);
			if (position.IsOpened()) {
				const auto price = position.GetType() == Position::TYPE_LONG
					?	PositionTrait<LongPosition>::GetCurrentClosePassivePrice(
							*m_security)
					:	PositionTrait<ShortPosition>::GetCurrentClosePassivePrice(
							*m_security);
				position.Close(Position::CLOSE_TYPE_TAKE_PROFIT, price);
			} else {
				position.CancelAllOrders();
			}
			timeMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_2);

		}

		void CheckSlowOrderExecution(const Milestones &timeMeasurement) {
			// if too slow: {
			// timeMeasurement.Measure(SM_STRATEGY_EXECUTION_START_2);
			//		cancel passive order and send aggressive here
			// timeMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_2);
			// } else
			{
				timeMeasurement.Measure(SM_STRATEGY_EXECUTION_START_2);
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

			switch (fastEmaDirection) {
				case  DIRECTION_LEVEL:
					// Intersection was at previous update.
					return DIRECTION_LEVEL;
				default:
					switch (m_fastEmaDirection) {
						case DIRECTION_UP:
						case DIRECTION_DOWN:
							return m_fastEmaDirection;
						case DIRECTION_LEVEL:
							return fastEmaDirection == DIRECTION_DOWN
								?	DIRECTION_UP
								:	DIRECTION_DOWN;
						default:
							throw LogicError(
								"Internal error: Unknown direction");
					}
			}

		}

		template<typename Position>
		boost::shared_ptr<Position> CreateOrder(
				const Milestones &timeMeasurement) {
			Assert(m_security);
			return boost::make_shared<Position>(
				*this,
				m_generateUuid(),
				0,
				GetTradeSystem(m_security->GetSource().GetIndex()),
				*m_security,
				m_security->GetSymbol().GetCurrency(),
				m_numberOfContracts,
				PositionTrait<Position>::GetCurrentOpenPassivePrice(*m_security),
				timeMeasurement);
		}

	private:

		const Qty m_numberOfContracts;

		Security *m_security;

		boost::array<Ema, numberOfEmaTypes> m_ema;
		Direction m_fastEmaDirection;

		boost::uuids::random_generator m_generateUuid;

	};

} } }

////////////////////////////////////////////////////////////////////////////////

TRDK_STRATEGY_GADM_API boost::shared_ptr<Strategy> CreateEmaFuturesStrategy(
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf) {
	using namespace trdk::Strategies::GadM;
	return boost::make_shared<EmaFuturesStrategy>(context, tag, conf);
}

////////////////////////////////////////////////////////////////////////////////
