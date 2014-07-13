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

		explicit Security(Context &, const Lib::Symbol &, bool isTestSource);

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
					const Level1TickValue &tick) {
			CheckLastTrade(time, tick);
			Base::AddLevel1Tick(time, tick);
		}
		void AddLevel1Tick(
					const boost::posix_time::ptime &time,
					const Level1TickValue &tick1,
					const Level1TickValue &tick2) {
			CheckLastTrade(time, tick1);
			CheckLastTrade(time, tick2);
			Base::AddLevel1Tick(time, tick1, tick2);
		}

		void AddBar(const Bar &bar) {
			Base::AddBar(bar);
		}

		void SetBrokerPosition(Qty qty, bool isInitial) {
			Base::SetBrokerPosition(qty, isInitial);
		}

		void SetAllImpliedVolatility(double value) {
			Base::SetLastImpliedVolatility(value);
			Base::SetBidImpliedVolatility(value);
			Base::SetAskImpliedVolatility(value);
		}

		void SetLastImpliedVolatility(double value) {
			Base::SetLastImpliedVolatility(value);
		}
		void SetBidImpliedVolatility(double value) {
			Base::SetBidImpliedVolatility(value);
		}
		void SetAskImpliedVolatility(double value) {
			Base::SetAskImpliedVolatility(value);
		}

	private:

		void CheckLastTrade(
					const boost::posix_time::ptime &time,
					const Level1TickValue &tick) {
			if (!m_isTestSource || !IsTradesRequired()) {
				return;
			}
			// real time bar not available from TWS demo account:
			switch (tick.type) {
				case LEVEL1_TICK_LAST_PRICE:
					{
						auto lastQty = GetLastQty();
						if (!lastQty) {
							if (!m_firstTradeTick) {
								m_firstTradeTick = tick;
								break;
							} else if (m_firstTradeTick->type == tick.type) {
								break;
							} else {
								AssertEq(
									LEVEL1_TICK_LAST_QTY,
									m_firstTradeTick->type);
								lastQty = m_firstTradeTick->value.lastQty;
							}
						}
						AddTrade(
							time,
							ORDER_SIDE_SELL,
							tick.value.lastPrice,
							lastQty,
							false,
							true);
					}
					break;
				case LEVEL1_TICK_LAST_QTY:
					{
						auto lastPrice = GetLastPriceScaled();
						if (!lastPrice) {
							if (!m_firstTradeTick) {
								m_firstTradeTick = tick;
								break;
							} else if (m_firstTradeTick->type == tick.type) {
								break;
							} else {
								AssertEq(
									LEVEL1_TICK_LAST_PRICE,
									m_firstTradeTick->type);
								lastPrice = m_firstTradeTick->value.lastPrice;
							}
						}
						AddTrade(
							time,
							ORDER_SIDE_BUY,
							lastPrice,
							tick.value.lastQty,
							false,
							true);
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
