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
#include "Core/PositionReporter.hpp"
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
					const std::string &tag)
				: Super(context, name, tag) {
			//...//
		}
		
		virtual ~TestStrategy() {
			//...//
		}

	public:

		virtual void ReportDecision(const Position &) const {
			//...//
		}

		virtual std::auto_ptr<PositionReporter> CreatePositionReporter()
				const {
			return std::auto_ptr<PositionReporter>();
		}

	public:
		
		virtual void OnLevel1Update(Security &security) {
			GetContext().GetLog().Debug(
				"%1% (%6%): bid = %2% / %3%, ask = %4% / %5%;",
				boost::make_tuple(
					security.GetSymbol(),
					security.GetBidPrice(),
					security.GetBidQty(),
					security.GetAskPrice(),
					security.GetAskQty(),
					security.GetSource().GetTag()));
			const auto &lastPrice = security.GetLastPriceScaled();
			if (		lastPrice > security.ScalePrice(10.99)
					|| lastPrice < security.ScalePrice(10.01)) {
				return;
			}
			const auto &priceToBuy = lastPrice - security.ScalePrice(.01);
			boost::shared_ptr<LongPosition> pos(
				new LongPosition(
					*this,
					security,
					CURRENCY_USD,
					40000,
					priceToBuy));
			pos->OpenOrCancel(priceToBuy);
		}
		
	protected:
		
		virtual void UpdateAlogImplSettings(const Lib::IniSectionRef &) {
			//...//
		}
		
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
			const trdk::Lib::IniSectionRef &) {
	return boost::shared_ptr<trdk::Strategy>(
		new trdk::Strategies::Test::TestStrategy(context, tag, "Test"));
}

////////////////////////////////////////////////////////////////////////////////
