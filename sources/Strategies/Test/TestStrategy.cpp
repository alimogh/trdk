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
#include "Core/DropCopy.hpp"

using namespace trdk;
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
			: Super(context, name, tag, conf)
			, m_numberOfOperations(0) {
			//...//
		}
		
		virtual ~TestStrategy() {
			//...//
		}

	protected:

		void OnPositionUpdate(Position &position) {

			Assert(m_operationId);
			if (position.IsError()) {
				Assert(IsBlocked());
				return;
			}
			if (position.HasActiveOrders()) {
				return;
			}
			Assert(!position.IsCompleted() || position.GetOpenedQty() == 0);

			DropCopy *const dropCopy = GetContext().GetDropCopy();
			if (dropCopy) {

				boost::shared_ptr<FinancialResult> financialResult(
					new FinancialResult);
				financialResult->push_back(std::make_pair(CURRENCY_AUD, 8877.111));
				financialResult->push_back(std::make_pair(CURRENCY_CHF, -679.12));
				financialResult->push_back(std::make_pair(CURRENCY_JPY, 787));

				dropCopy->ReportOperationEnd(
					*m_operationId,
					GetContext().GetCurrentTime(),
					9.99,
					financialResult);

			}

			m_operationId.reset();

		}

		virtual void OnBookUpdateTick(
				Security &security,
				const Security::Book &book,
				const TimeMeasurement::Milestones &timeMeasurement) {

			if (m_numberOfOperations > 5) {
				return;
			}

			if (!book.GetAsks().GetSize() || !book.GetBids().GetSize()) {
				return;
			}

			if (m_operationId) {
				return;
			}

			const auto &operationId = m_generateUuid();

			const auto price = m_numberOfOperations % 2
				?	book.GetAsks().GetLevel(0).GetPrice()
				:	book.GetBids().GetLevel(0).GetPrice();
			Assert(!IsZero(price));

			boost::shared_ptr<Position> position;
			if (m_numberOfOperations % 2) {
				position.reset(
					new LongPosition(
						*this,
						operationId,
						m_numberOfOperations + 111,
						GetTradeSystem(security.GetSource().GetIndex()),
						security,
						security.GetSymbol().GetFotBaseCurrency(),
						100000,
						security.ScalePrice(price),
						timeMeasurement));
			} else {
				position.reset(
					new ShortPosition(
						*this,
						operationId,
						m_numberOfOperations + 111,
						GetTradeSystem(security.GetSource().GetIndex()),
						security,
						security.GetSymbol().GetFotBaseCurrency(),
						100000,
						security.ScalePrice(price),
						timeMeasurement));
			}

			DropCopy *const dropCopy = GetContext().GetDropCopy();
			if (dropCopy) {
				dropCopy->ReportOperationStart(
					operationId,
					GetContext().GetCurrentTime(),
					*this,
					999111999);
			}

			position->OpenImmediatelyOrCancel(security.ScalePrice(price));

			m_operationId = operationId;
			++m_numberOfOperations;

		}

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
					1000000,
					priceToBuy,
					timeMeasurement));
			pos->OpenAtMarketPrice();

		}

		virtual void OnPostionsCloseRequest() {
			GetContext().GetLog().Debug("All positions closed.");
		}

	private:

		boost::uuids::random_generator m_generateUuid;

		size_t m_numberOfOperations;

		boost::optional<boost::uuids::uuid> m_operationId;
		
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
		new trdk::Strategies::Test::TestStrategy(context, "Test", tag, conf));
}

////////////////////////////////////////////////////////////////////////////////
