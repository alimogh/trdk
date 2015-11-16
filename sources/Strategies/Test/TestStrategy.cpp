/**************************************************************************
 *   Created: 2014/08/07 12:39:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Common/Assert.hpp"
#include "Common/Common.hpp"
#include "Core/Strategy.hpp"
#include "Core/Security.hpp"
#include "Core/Position.hpp"
#include "Core/MarketDataSource.hpp"

using namespace trdk::Lib;

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Strategies { namespace Test {
	
	class TestStrategy : public Strategy {
		
	public:
		
		typedef Strategy Super;

	public:

		explicit TestStrategy(
				Context &context,
				const std::string &name,
				const std::string &tag,
				const Lib::IniSectionRef &conf)
			: Super(context, name, tag, conf) {
			//...//
		}
		
		virtual ~TestStrategy() {
			//...//
		}

	public:
		
		virtual void OnLevel1Update(
				Security &security,
				const Lib::TimeMeasurement::Milestones &timeMeasurement) {

			GetContext().GetLog().Debug(
				"%1% (%6%): bid = %2% / %3%, ask = %4% / %5%;",
				security.GetSymbol(),
				security.GetBidPrice(),
				security.GetBidQty(),
				security.GetAskPrice(),
				security.GetAskQty(),
				security.GetSource().GetTag());
			const auto &lastPrice = security.GetLastPriceScaled();
			if (	
					lastPrice > security.ScalePrice(10.99)
					|| lastPrice < security.ScalePrice(10.01)) {
				return;
			}
			const auto &priceToBuy = lastPrice - security.ScalePrice(.01);
			boost::shared_ptr<LongPosition> pos(
				new LongPosition(
					*this,
					m_generateUuid(),
					999,
					GetTradeSystem(0),
					security,
					CURRENCY_EUR,
					Qty(1000000),
					priceToBuy,
					timeMeasurement));
			pos->OpenAtMarketPrice();

		}
		
		virtual void OnBookUpdateTick(
				trdk::Security &,
				const trdk::Security::Book &,
				const trdk::Lib::TimeMeasurement::Milestones &) {
			//...//
		}

	protected:

		virtual void OnPostionsCloseRequest() {
			GetContext().GetLog().Debug("All positions closed.");
		}

	private:

		boost::uuids::random_generator m_generateUuid;
		
	};
	
} } }


////////////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
#	define TRDK_STRATEGY_TEST_API
#else
#	define TRDK_STRATEGY_TEST_API extern "C"
#endif

TRDK_STRATEGY_TEST_API boost::shared_ptr<trdk::Strategy> CreateStrategy(
			trdk::Context &context,
			const std::string &tag,
			const trdk::Lib::IniSectionRef &conf) {
	return boost::shared_ptr<trdk::Strategy>(
		new trdk::Strategies::Test::TestStrategy(context, tag, "Test", conf));
}

////////////////////////////////////////////////////////////////////////////////
