/**************************************************************************
 *   Created: 2013/05/01 16:43:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace trdk {  namespace Interaction { namespace InteractiveBrokers {


	class Security : public trdk::Security {

	public:

		typedef trdk::Security Base;

	public:

		explicit Security(
				Context &,
				const Lib::Symbol &,
				MarketDataSource &,
				bool isTestSource);

	public:

		bool IsTestSource() const {
			return m_isTestSource;
		}

		bool IsLevel1Required() const {
			return Base::IsLevel1Required();
		}
		bool IsLevel1UpdatesRequired() const {
			return Base::IsLevel1UpdatesRequired();
		}
		bool IsLevel1TicksRequired() const {
			return Base::IsLevel1TicksRequired();
		}
		bool IsTradesRequired() const {
			return Base::IsTradesRequired();
		}
		bool IsBrokerPositionRequired() const {
			return Base::IsBrokerPositionRequired();
		}
		bool IsBarsRequired() const {
			return Base::IsBarsRequired();
		}

		void AddLevel1Tick(
				const boost::posix_time::ptime &time,
				const Level1TickValue &tick,
				const Lib::TimeMeasurement::Milestones &timeMeasurement) {
			CheckLastTrade(time, tick, timeMeasurement);
			Base::AddLevel1Tick(time, tick, timeMeasurement);
		}
		void AddLevel1Tick(
				const boost::posix_time::ptime &time,
				const Level1TickValue &tick1,
				const Level1TickValue &tick2,
				const Lib::TimeMeasurement::Milestones &timeMeasurement) {
			CheckLastTrade(time, tick1, timeMeasurement);
			CheckLastTrade(time, tick2, timeMeasurement);
			Base::AddLevel1Tick(time, tick1, tick2, timeMeasurement);
		}
		void AddLevel1Tick(
				const boost::posix_time::ptime &time,
				const Level1TickValue &tick1,
				const Level1TickValue &tick2,
				const Level1TickValue &tick3,
				const Level1TickValue &tick4,
				const Lib::TimeMeasurement::Milestones &timeMeasurement) {
			CheckLastTrade(time, tick1, timeMeasurement);
			CheckLastTrade(time, tick2, timeMeasurement);
			CheckLastTrade(time, tick3, timeMeasurement);
			CheckLastTrade(time, tick4, timeMeasurement);
			Base::AddLevel1Tick(
				time,
				tick1,
				tick2,
				tick3,
				tick4,
				timeMeasurement);
		}


		void AddBar(Bar &&bar) {
			Base::AddBar(std::move(bar));
		}

		void SetBrokerPosition(Qty qty, bool isInitial) {
			Base::SetBrokerPosition(qty, isInitial);
		}

		using Base::SetExpiration;
		using Base::SetOnline;
		using Base::SetTradingSessionState;

	private:

		void CheckLastTrade(
				const boost::posix_time::ptime &time,
				const Level1TickValue &tick,
				const Lib::TimeMeasurement::Milestones &timeMeasurement) {
			if (!m_isTestSource || !IsTradesRequired()) {
				return;
			}
			// real time bar not available from TWS demo account:
			switch (tick.GetType()) {
				case LEVEL1_TICK_LAST_PRICE:
					{
						auto lastQty = GetLastQty();
						if (Lib::IsZero(lastQty)) {
							if (!m_firstTradeTick) {
								m_firstTradeTick = tick;
								break;
							} else if (m_firstTradeTick->GetType() == tick.GetType()) {
								break;
							} else {
								AssertEq(
									LEVEL1_TICK_LAST_QTY,
									m_firstTradeTick->GetType());
								lastQty = m_firstTradeTick->GetValue();
							}
						}
						AddTrade(
							time,
							ScaledPrice(tick.GetValue()),
							lastQty,
							timeMeasurement,
							false);
					}
					break;
				case LEVEL1_TICK_LAST_QTY:
					{
						auto lastPrice = GetLastPriceScaled();
						if (Lib::IsZero(lastPrice)) {
							if (!m_firstTradeTick) {
								m_firstTradeTick = tick;
								break;
							} else if (m_firstTradeTick->GetType() == tick.GetType()) {
								break;
							} else {
								AssertEq(
									LEVEL1_TICK_LAST_PRICE,
									m_firstTradeTick->GetType());
								lastPrice
									= ScaledPrice(m_firstTradeTick->GetValue());
							}
						}
						AddTrade(
							time,
							lastPrice,
							tick.GetValue(),
							timeMeasurement,
							false);
					}
					break;
				default:
					break;
			}
		}
		
	private:

		const bool m_isTestSource;
		boost::optional<Level1TickValue> m_firstTradeTick;

	};

} } }
