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
#include "Core/PriceBook.hpp"
#include "Core/Position.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/DropCopy.hpp"
#include "Api.h"

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;
namespace uuids = boost::uuids;

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
			: Super(
				context,
				uuids::string_generator()(
					"{063AB9A2-EE3E-4AF7-85B0-AC0B63E27F43}"),
				name,
				tag,
				conf)
			, m_numberOfOperations(0)
			, m_finResultRange(0, 1000)
			, m_generateFinResultRandom(m_random, m_finResultRange) {
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

			if (position.GetOpenedQty()) {

				GetContext().InvokeDropCopy(
					[this, &position](DropCopy &dropCopy) {

						FinancialResult financialResult;
						financialResult.emplace_back(
							std::make_pair(
								position
									.GetSecurity()
									.GetSymbol()
									.GetFotBaseCurrency(),
								m_generateFinResultRandom()));
						financialResult.emplace_back(
							std::make_pair(
								position
									.GetSecurity()
									.GetSymbol()
									.GetFotQuoteCurrency(),
								m_generateFinResultRandom()));

						dropCopy.ReportOperationEnd(
							*m_operationId,
							GetContext().GetCurrentTime(),
							m_generateFinResultRandom() >= 800
								?	OPERATION_RESULT_LOSS
								:	OPERATION_RESULT_PROFIT,
							m_generateFinResultRandom()
								/ m_generateFinResultRandom(),
							std::move(financialResult));

					});

			}

			m_operationId.reset();

		}

		virtual void OnBookUpdateTick(
				Security &security,
				const PriceBook &book,
				const TimeMeasurement::Milestones &timeMeasurement) {

			if (m_operationId) {
				return;
			}

			const auto &now = GetContext().GetCurrentTime();
			if (
					m_lastOperationTime != pt::not_a_date_time
					&& now - m_lastOperationTime > pt::minutes(3)) {
				return;
			}

			if (book.GetAsk().IsEmpty() || book.GetBid().IsEmpty()) {
				return;
			}

			const auto &operationId = m_generateUuid();

			const auto price = m_numberOfOperations % 2
				?	book.GetAsk().GetTop().GetPrice()
				:	book.GetBid().GetTop().GetPrice();
			Assert(!IsZero(price));

			boost::shared_ptr<Position> position;
			if (m_numberOfOperations % 2) {
				position.reset(
					new LongPosition(
						*this,
						operationId,
						m_numberOfOperations + 111,
						GetTradingSystem(security.GetSource().GetIndex()),
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
						m_numberOfOperations + 222,
						GetTradingSystem(security.GetSource().GetIndex()),
						security,
						security.GetSymbol().GetFotBaseCurrency(),
						200000,
						security.ScalePrice(price),
						timeMeasurement));
			}

			GetContext().InvokeDropCopy(
				[this, &operationId](DropCopy &dropCopy) {
					dropCopy.ReportOperationStart(
						*this,
						operationId,
						GetContext().GetCurrentTime());
				});

			position->OpenImmediatelyOrCancel(security.ScalePrice(price));

			m_operationId = operationId;
			++m_numberOfOperations;

			m_lastOperationTime = now;

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
					GetTradingSystem(0),
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
		pt::ptime m_lastOperationTime;

		boost::mt19937 m_random;
		boost::uniform_int<uint32_t> m_finResultRange;
		boost::variate_generator<boost::mt19937, boost::uniform_int<uint32_t>>
			m_generateFinResultRandom;

	};
	
} } }


////////////////////////////////////////////////////////////////////////////////

TRDK_STRATEGY_TEST_API boost::shared_ptr<trdk::Strategy> CreateStrategy(
			trdk::Context &context,
			const std::string &tag,
			const trdk::Lib::IniSectionRef &conf) {
	return boost::shared_ptr<trdk::Strategy>(
		new trdk::Strategies::Test::TestStrategy(context, "Test", tag, conf));
}

////////////////////////////////////////////////////////////////////////////////
