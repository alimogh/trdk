/**************************************************************************
 *   Created: 2012/09/11 18:52:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"
#include "Core/Security.hpp"
#include "Core/Context.hpp"

namespace trdk { namespace Interaction { namespace Fake {

	class MarketDataSource : public trdk::MarketDataSource {

	public:

		typedef trdk::MarketDataSource Base;

		class Security : public trdk::Security {

		public:

			typedef trdk::Security Base;

		public:
				
			explicit Security(
						Context &context,
						const Lib::Symbol &symbol,
						const MarketDataSource &source)
					: Base(context, symbol, source) {
				StartLevel1();
			}

			void AddTrade(
						const boost::posix_time::ptime &time,
						const OrderSide &side,
						const ScaledPrice &price,
						const Qty &qty,
						const Lib::TimeMeasurement::Milestones &timeMeasurement) {
				Base::AddTrade(
					time,
					side,
					price,
					qty,
					timeMeasurement,
					true,
					true);
			}

			void AddBar(const Bar &bar) {
				Base::AddBar(bar);
			}

			void SetLevel1(
						double bidPrice,
						const Qty &bidQty,
						double askPrice,
						const Qty &askQty,
						const Lib::TimeMeasurement::Milestones &timeMeasurement) {
				Base::SetLevel1(
					boost::posix_time::microsec_clock::universal_time(),
					Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(
						ScalePrice(askPrice)),
					Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(askQty),
					timeMeasurement);
				Base::SetLevel1(
					boost::posix_time::microsec_clock::universal_time(),
					Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(
						ScalePrice(bidPrice)),
					Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(bidQty),
					timeMeasurement);
			}

		};

	public:

		MarketDataSource(
					Context &context,
					const std::string &tag,
					const Lib::IniSectionRef &);
		virtual ~MarketDataSource();

	public:

		virtual void Connect(const trdk::Lib::IniSectionRef &);

		virtual void SubscribeToSecurities() {
			//...//
		}

	protected:

		virtual boost::shared_ptr<trdk::Security> CreateSecurity(
					Context &,
					const trdk::Lib::Symbol &)
				const;

	private:

		void NotificationThread();

	private:

		boost::thread_group m_threads;
		boost::atomic_bool m_stopFlag;

		std::list<boost::shared_ptr<Security>> m_securityList;

	};

} } }
