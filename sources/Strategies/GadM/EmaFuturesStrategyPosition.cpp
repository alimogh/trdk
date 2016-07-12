/**************************************************************************
 *   Created: 2016/04/14 21:21:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "EmaFuturesStrategyPosition.hpp"
#include "Core/Strategy.hpp"
#include "Core/DropCopy.hpp"
#include "Core/RiskControl.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Strategies::GadM;
using namespace trdk::Strategies::GadM::EmaFuturesStrategy;

namespace ids = boost::uuids;
namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

EmaFuturesStrategy::Position::Position(
		const Direction &openReason,
		const SlowFastEmas &emas,
		std::ostream &reportStream)
	: m_intention(INTENTION_OPEN_PASSIVE)
	, m_isSent(false)
	, m_isPassiveOpen(true)
	, m_isPassiveClose(true)
	, m_emas(emas)
	, m_closeType(CLOSE_TYPE_NONE)
	, m_maxProfit(0)
	, m_reportStream(reportStream) {

	m_reasons[0] = openReason;
	AssertNe(DIRECTION_LEVEL, m_reasons[0]);
	m_reasons[1] = DIRECTION_LEVEL;

	m_signalsBidAsk[0] = std::make_pair(
		GetSecurity().GetBidPrice(),
		GetSecurity().GetAskPrice());
	m_signalsBidAsk[1] = std::make_pair(.0, .0);

	m_signalsEmas[0] = std::make_pair(
		m_emas[SLOW].GetValue(),
		m_emas[FAST].GetValue());
	m_signalsEmas[1] = std::make_pair(.0, .0);

}

EmaFuturesStrategy::Position::~Position() {
	//...//
}

const EmaFuturesStrategy::Position::Time &
EmaFuturesStrategy::Position::GetStartTime() const {
	return m_startTime;
}

const EmaFuturesStrategy::Position::Time &
EmaFuturesStrategy::Position::GetCloseStartTime() const {
	AssertNe(pt::not_a_date_time, m_closeStartTime);
	return m_closeStartTime;
}

const Direction & EmaFuturesStrategy::Position::GetOpenReason() const {
	return m_reasons[0];
}

const Intention & EmaFuturesStrategy::Position::GetIntention() const {
	return m_intention;
}

void EmaFuturesStrategy::Position::SetIntention(
		Intention intention,
		const CloseType &closeType,
		const Direction &closeReason) {

	AssertNe(m_intention, intention);
	AssertNe(INTENTION_OPEN_PASSIVE, intention);
	GetStrategy().GetTradingLog().Write(
		"intention\t%1%->%2%\t%3%\t%4%\t%5%\t%6%\t%7%",
		[&](TradingRecord &record) {
			record
				% m_intention
				% intention
				% (m_isPassiveOpen ? "true" : "false")
				% (m_isPassiveClose ? "true" : "false")
				% (m_isSent ? "true" : "false")
				% closeType
				% closeReason;
		});

	if (closeType != CLOSE_TYPE_NONE) {
		m_closeType = closeType;
	}
	if (closeReason != DIRECTION_LEVEL) {
		m_reasons[1] = closeReason;
	}

	Sync(intention);

	m_intention = intention;

}

void EmaFuturesStrategy::Position::Sync() {
	Sync(m_intention);
}

void EmaFuturesStrategy::Position::MoveOrderToCurrentPrice() {
	Assert(HasActiveOrders());
	if (!m_isSent) {
		GetStrategy().GetTradingLog().Write(
			"move\tis already started\t%1%\t%2%\t%3%",
			[&](TradingRecord &record){
				record % m_intention % m_closeType % m_reasons[1];
			});
		return;
	}
	m_isSent = false;
	switch (m_intention) {
		case INTENTION_OPEN_PASSIVE:
			m_intention = INTENTION_OPEN_AGGRESIVE;
			GetStrategy().GetTradingLog().Write(
				"move\t%1%->%2%\t%3%\t%4%",
				[&](TradingRecord &record) {
				record
					% INTENTION_OPEN_PASSIVE
					% m_intention
					% m_closeType
					% m_reasons[1];
			});
			break;
		case INTENTION_CLOSE_PASSIVE:
			m_intention = INTENTION_CLOSE_AGGRESIVE;
			GetStrategy().GetTradingLog().Write(
				"move\t%1%->%2%\t%3%\t%4%",
				[&](TradingRecord &record) {
					record
						% INTENTION_CLOSE_PASSIVE
						% m_intention
						% m_closeType
						% m_reasons[1];
				});
			break;
		default:
			GetStrategy().GetTradingLog().Write(
				"move\t%1%\t%2%\t%3%",
				[&](TradingRecord &record) {
					record % m_intention % m_closeType % m_reasons[1];
				});
	}
	CancelAllOrders();
}

void EmaFuturesStrategy::Position::Sync(Intention &intention) {

	static_assert(numberOfIntentions == 6, "List changed.");
	switch (intention) {

		case INTENTION_OPEN_PASSIVE:
			Assert(m_isPassiveOpen);
			if (IsOpened()) {
				intention = INTENTION_HOLD;
				m_isSent = false;
			} else if (HasActiveOrders()) {
				//...//
			} else if (m_isSent) {
				throw Exception(
					"Order canceled by trading system without request");
			} else {
				const auto startTime
					= GetStrategy().GetContext().GetCurrentTime();
				GetStrategy().GetContext().InvokeDropCopy(
					[this, &startTime](DropCopy &dropCopy) {
					dropCopy.ReportOperationStart(
						GetId(),
						startTime,
						GetStrategy());
				});
				Open(GetOpenStartPrice());
				m_isPassiveOpen = true;
				m_isSent = true;
				m_startTime = std::move(startTime);
			}
			break;

		case INTENTION_DONOT_OPEN:
			Assert(!IsOpened());
			if (HasActiveOpenOrders()) {
				CancelAllOrders();
				m_isSent = false;
			} else if (m_isSent) {
				throw Exception(
					"Order canceled by trading system without request");
			} else {
				intention = INTENTION_HOLD;
				m_isSent = false;
			}
			break;

		case INTENTION_OPEN_AGGRESIVE:
			if (IsOpened()) {
				intention = INTENTION_HOLD;
				m_isSent = false;
			} else if (HasActiveOrders()) {
				if (m_isPassiveOpen) {
					CancelAllOrders();
					m_isSent = false;
				}
			} else if (m_isSent) {
				throw Exception(
					"Order canceled by trading system without request");
			} else {
				Open(GetMarketOpenPrice());
				m_isPassiveOpen = false;
				m_isSent = true;
			}
			break;

		case INTENTION_HOLD:
			Assert(!m_isSent);
			break;

		case INTENTION_CLOSE_PASSIVE:
			Assert(IsOpened());
			Assert(m_isPassiveClose);
			Assert(!HasActiveOpenOrders());
			if (IsCompleted()) {
				intention = INTENTION_HOLD;
			} else if (HasActiveOrders()) {
				Assert(!HasActiveOpenOrders());
			} else if (m_isSent) {
				throw Exception(
					"Order canceled by trading system without request");
			} else {

				SetCloseStartPrice(GetPassiveClosePrice());
				AssertEq(m_closeStartTime, pt::not_a_date_time);
				m_closeStartTime = GetStrategy().GetContext().GetCurrentTime();

				Assert(IsZero(m_signalsBidAsk[1].first));
				Assert(IsZero(m_signalsBidAsk[1].second));
				m_signalsBidAsk[1] = std::make_pair(
					GetSecurity().GetBidPrice(),
					GetSecurity().GetAskPrice());

				Assert(IsZero(m_signalsEmas[1].first));
				Assert(IsZero(m_signalsEmas[1].second));
				m_signalsEmas[1] = std::make_pair(
					m_emas[SLOW].GetValue(),
					m_emas[FAST].GetValue());

				Close(m_closeType, GetCloseStartPrice());
				m_isPassiveClose = true;
				m_isSent = true;

			}
			break;

		case INTENTION_CLOSE_AGGRESIVE:
			Assert(IsOpened());
			Assert(!HasActiveOpenOrders());
			if (IsCompleted()) {
				intention = INTENTION_HOLD;
			} else if (HasActiveOrders()) {
				Assert(!HasActiveOpenOrders());
				if (m_isPassiveClose) {
					CancelAllOrders();
					m_isSent = false;
				}
			} else if (m_isSent) {
				throw Exception(
					"Order canceled by trading system without request");
			} else {
				SetCloseStartPrice(GetMarketClosePrice());
				Close(m_closeType, GetCloseStartPrice());
				m_isPassiveClose = false;
				m_isSent = true;
			}
			break;
		
		default:
			AssertEq(INTENTION_OPEN_PASSIVE, intention);
			break;
	
	}

}

EmaFuturesStrategy::Position::PriceCheckResult
EmaFuturesStrategy::Position::CheckTakeProfit(
		double minProfit,
		double trailingPercentage) {
	PriceCheckResult result = {};
	result.current = CaclCurrentProfit();
	if (result.current > m_maxProfit) {
		m_maxProfit = result.current;
	}
	result.start = m_maxProfit;
	result.margin = ScaledPrice(m_maxProfit * trailingPercentage);
	const ScaledPrice minProfitVol
		= GetSecurity().ScalePrice(minProfit) * ScaledPrice(GetActiveQty());
	result.isAllowed
		= m_maxProfit < minProfitVol
		|| result.current > result.margin;
	return result;
}

void EmaFuturesStrategy::Position::OpenReport(std::ostream &reportStream) {
	reportStream
		/* 1	*/ << "Open Start Time"
		/* 2	*/ << ",Open Time"
		/* 3	*/ << ",Opening Duration"
		/* 4	*/ << ",Close Start Time"
		/* 5	*/ << ",Close Time"
		/* 6	*/ << ",Closing Duration"
		/* 7	*/ << ",Position Duration"
		/* 8	*/ << ",Type"
		/* 9	*/ << ",P&L"
		/* 10	*/ << ",Qty"
		/* 11	*/ << ",Entry Reason"
		/* 12	*/ << ",Entry Price"
		/* 13	*/ << ",Entry Volume"
		/* 14	*/ << ",Entry Sig. Bid"
		/* 15	*/ << ",Entry Sig. Ask"
		/* 16	*/ << ",Entry Sig. Slow EMA"
		/* 17	*/ << ",Entry Sig. Fast EMA"
		/* 18	*/ << ",Exit Reason"
		/* 19	*/ << ",Exit Price"
		/* 20	*/ << ",Exit Volume"
		/* 21	*/ << ",Exit Sig. Bid"
		/* 22	*/ << ",Exit Sig. Ask"
		/* 23	*/ << ",Exit Sig. Slow EMA"
		/* 24	*/ << ",Exit Sig. Fast EMA"
		/* 25	*/ << ",ID"
		<< std::endl;
}

void EmaFuturesStrategy::Position::Report() throw() {

	if (!GetOpenedQty() || !IsCompleted()) {
		return;
	}

	try {
		GetStrategy().GetContext().InvokeDropCopy(
			[this](DropCopy &dropCopy) {
				const double pnl = GetType() == TYPE_LONG
					? GetOpenedVolume() / GetClosedVolume()
					: GetClosedVolume() / GetOpenedVolume();
				dropCopy.ReportOperationEnd(
					GetId(),
					GetCloseTime(),
					IsEqual(pnl, 1.0) || pnl < 1.0
						? OPERATION_RESULT_LOSS
						: OPERATION_RESULT_PROFIT,
					pnl,
					boost::make_shared<FinancialResult>());
			});
	} catch (const std::exception &ex) {
		GetStrategy().GetLog().Error(
			"Failed to report operation end: \"%1%\".",
			ex.what());
	} catch (...) {
		AssertFailNoException();
		terminate();
	}

	try {

		// 1. Open Start Time, 2. Open Time, 3. Opening Duration:
		m_reportStream
			<< GetStartTime()
			<< ',' << GetOpenTime()
			<< ',' << (GetOpenTime() - GetStartTime());

		// 4. Close Start Time, 5. Close Time, 6. Closing Duration:
		m_reportStream
			<< ',' << GetCloseStartTime()
			<< ',' << GetCloseTime()
			<< ',' << (GetCloseTime() - GetCloseStartTime());

		// 7. Position Duration:
		m_reportStream << ',' << (GetCloseTime() - GetOpenTime());

		// 8. Type:
		m_reportStream << ',' << GetTypeStr();

		// 8. pnl:
		m_reportStream << ',';
		m_reportStream << (GetType() == TYPE_LONG
			?	GetClosedVolume() - GetOpenedVolume()
			:	GetOpenedVolume() - GetClosedVolume());

		// 9. qty:
		m_reportStream << ',' << GetOpenedQty();

		// 10. entry reason:
		m_reportStream << ',';
		switch (m_reasons[0]) {
			default:
			case DIRECTION_LEVEL:
				AssertEq(DIRECTION_UP, m_reasons[0]);
				m_reportStream << '-';
				break;
			case DIRECTION_UP:
				m_reportStream << "fast EMA grows";
				break;
			case DIRECTION_DOWN:
				m_reportStream << "fast EMA falls";
				break;
		}

		// 11. entry price, 12. entry volume
		m_reportStream
			<< ',' << GetSecurity().DescalePrice(GetOpenPrice())
			<< ',' << GetOpenedVolume();
		
		// 13. entry bid, 14. entry ask,
		m_reportStream
			<< ',' << m_signalsBidAsk[0].first
			<< ',' << m_signalsBidAsk[0].second;

		// 15. entry slow ema, 16. entry fast ema
		m_reportStream
			<< ',' << m_signalsEmas[0].first
			<< ',' << m_signalsEmas[0].second;

		// 17. exit reason:
		m_reportStream << ',';
		switch (m_reasons[1]) {
			default:
				AssertEq(DIRECTION_UP, m_reasons[1]);
			case DIRECTION_LEVEL:
				m_reportStream << GetCloseTypeStr();
				break;
			case DIRECTION_UP:
				m_reportStream << "fast EMA grows";
				break;
			case DIRECTION_DOWN:
				m_reportStream << "fast EMA falls";
				break;
		}

		// 18. exit price, 19. exit volume,
		m_reportStream
			<< ',' << GetSecurity().DescalePrice(GetClosePrice())
			<< ',' << GetClosedVolume();

		// 20. exit bid, 21. exit ask,
		m_reportStream
			<< ',' << m_signalsBidAsk[1].first
			<< ',' << m_signalsBidAsk[1].second;
	
		// 22. exit slow ema, 23. exit fast ema.
		m_reportStream
			<< ',' << m_signalsEmas[1].first
			<< ',' << m_signalsEmas[1].second;

		// 24. ID
		m_reportStream << ',' << GetId();

		m_reportStream << std::endl;


	} catch (const std::exception &) {
		AssertFailNoException();
	} catch (...) {
		AssertFailNoException();
		terminate();
	}

}

////////////////////////////////////////////////////////////////////////////////

EmaFuturesStrategy::LongPosition::LongPosition(
		Strategy &startegy,
		const ids::uuid &operationId,
		TradingSystem &tradingSystem,
		Security &security,
		const Qty &qty,
		const Milestones &strategyTimeMeasurement,
		const Direction &openReason,
		const SlowFastEmas &emas,
		std::ostream &reportStream)
	: trdk::Position(
		startegy,
		operationId,
		1,
		tradingSystem,
		security,
		security.GetSymbol().GetCurrency(),
		qty,
		security.GetBidPriceScaled(),
		strategyTimeMeasurement)
	, Position(openReason, emas, reportStream) {
	//...//
}

EmaFuturesStrategy::LongPosition::~LongPosition() {
	Report();
}

ScaledPrice EmaFuturesStrategy::LongPosition::GetMarketOpenPrice() const {
	return GetSecurity().GetAskPriceScaled();
}

ScaledPrice EmaFuturesStrategy::LongPosition::GetMarketClosePrice() const {
	return GetSecurity().GetBidPriceScaled();
}

ScaledPrice EmaFuturesStrategy::LongPosition::GetPassiveOpenPrice() const {
	return GetSecurity().GetBidPriceScaled();
}

ScaledPrice EmaFuturesStrategy::LongPosition::GetPassiveClosePrice() const {
	return GetSecurity().GetAskPriceScaled();
}

EmaFuturesStrategy::LongPosition::PriceCheckResult
EmaFuturesStrategy::LongPosition::CheckOrderPrice(double priceDelta) const {
	Assert(HasActiveOrders());
	PriceCheckResult result = {};
	if (HasActiveOpenOrders()) {
		result.start = GetOpenStartPrice();
		result.margin = result.start + GetSecurity().ScalePrice(priceDelta);
		result.current = GetIntention() == INTENTION_OPEN_PASSIVE
			?	GetPassiveOpenPrice()
			:	GetMarketOpenPrice();
		result.isAllowed = result.margin >= result.current;
	} else {
		result.start = GetCloseStartPrice();
		result.margin = result.start - GetSecurity().ScalePrice(priceDelta);
		result.current = GetIntention() == INTENTION_CLOSE_PASSIVE
			?	GetPassiveClosePrice()
			:	GetMarketClosePrice();
		result.isAllowed = result.margin <= result.current;
	}
	Assert(!IsZero(result.start));
	Assert(!IsZero(result.margin));
	Assert(!IsZero(result.current));
	return result;
}

EmaFuturesStrategy::LongPosition::PriceCheckResult
EmaFuturesStrategy::LongPosition::CheckStopLossByPrice(
		double priceDelta)
		const {
	Assert(!IsZero(GetOpenPrice()));
	AssertEq(INTENTION_HOLD, GetIntention());
	PriceCheckResult result = {};
	result.start = GetOpenPrice();
	result.margin = result.start - GetSecurity().ScalePrice(priceDelta);;
	result.current = GetMarketClosePrice();
	result.isAllowed = result.current >= result.margin;
	return result;
}

EmaFuturesStrategy::LongPosition::PriceCheckResult
EmaFuturesStrategy::LongPosition::CheckStopLossByLoss(
		double maxLossMoneyPerContract)
		const {
	Assert(!IsZero(GetOpenPrice()));
	AssertEq(INTENTION_HOLD, GetIntention());
	PriceCheckResult result = {};
	result.start = ScaledPrice(GetOpenPrice() * GetActiveQty());
	result.margin
		= result.start
		- (ScaledPrice(GetActiveQty())
		* GetSecurity().ScalePrice(maxLossMoneyPerContract));
	result.current = ScaledPrice(GetActiveQty() * GetMarketClosePrice());
	result.isAllowed = result.current >= result.margin;
	return result;
}

ScaledPrice EmaFuturesStrategy::LongPosition::CaclCurrentProfit() const {
	Assert(!IsZero(GetOpenPrice()));
	Assert(!IsZero(GetActiveQty()));
	AssertEq(INTENTION_HOLD, GetIntention());
	return 
		ScaledPrice(GetOpenPrice() * GetActiveQty())
		- ScaledPrice(GetActiveQty() * GetMarketClosePrice());
}

////////////////////////////////////////////////////////////////////////////////

EmaFuturesStrategy::ShortPosition::ShortPosition(
		Strategy &startegy,
		const ids::uuid &operationId,
		TradingSystem &tradingSystem,
		Security &security,
		const Qty &qty,
		const Milestones &strategyTimeMeasurement,
		const Direction &openReason,
		const SlowFastEmas &emas,
		std::ostream &reportStream)
	: trdk::Position(
		startegy,
		operationId,
		1,
		tradingSystem,
		security,
		security.GetSymbol().GetCurrency(),
		qty,
		security.GetAskPriceScaled(),
		strategyTimeMeasurement)
	, Position(openReason, emas, reportStream) {
	//...//
}

EmaFuturesStrategy::ShortPosition::~ShortPosition() {
	Report();
}

ScaledPrice EmaFuturesStrategy::ShortPosition::GetMarketOpenPrice() const {
	return GetSecurity().GetBidPriceScaled();
}

ScaledPrice EmaFuturesStrategy::ShortPosition::GetMarketClosePrice() const {
	return GetSecurity().GetAskPriceScaled();
}

ScaledPrice EmaFuturesStrategy::ShortPosition::GetPassiveOpenPrice() const {
	return GetSecurity().GetAskPriceScaled();
}

ScaledPrice EmaFuturesStrategy::ShortPosition::GetPassiveClosePrice() const {
	return GetSecurity().GetBidPriceScaled();
}

EmaFuturesStrategy::ShortPosition::PriceCheckResult
EmaFuturesStrategy::ShortPosition::CheckOrderPrice(
		double priceDelta)
		const {
	Assert(HasActiveOrders());
	PriceCheckResult result = {};
	if (HasActiveOpenOrders()) {
		result.start = GetOpenStartPrice();
		result.margin = result.start - GetSecurity().ScalePrice(priceDelta);
		result.current = GetIntention() == INTENTION_OPEN_PASSIVE
			?	GetPassiveOpenPrice()
			:	GetMarketOpenPrice();
		result.isAllowed = result.margin <= result.current;
	} else {
		result.start = GetCloseStartPrice();
		result.margin = result.start + GetSecurity().ScalePrice(priceDelta);
		result.current = GetIntention() == INTENTION_CLOSE_PASSIVE
			?	GetPassiveClosePrice()
			:	GetMarketClosePrice();
		result.isAllowed = result.margin >= result.current;
	}
	Assert(!IsZero(result.start));
	Assert(!IsZero(result.margin));
	Assert(!IsZero(result.current));
	return result;
}

EmaFuturesStrategy::ShortPosition::PriceCheckResult
EmaFuturesStrategy::ShortPosition::CheckStopLossByPrice(
		double priceDelta)
		const {
	Assert(!IsZero(GetOpenPrice()));
	AssertEq(INTENTION_HOLD, GetIntention());
	PriceCheckResult result = {};
	result.start = GetOpenPrice();
	result.margin = result.start + GetSecurity().ScalePrice(priceDelta);
	result.current = GetMarketClosePrice();
	result.isAllowed = result.current <= result.margin;
	return result;
}

EmaFuturesStrategy::ShortPosition::PriceCheckResult
EmaFuturesStrategy::ShortPosition::CheckStopLossByLoss(
		double maxLossMoneyPerContract)
		const {
	Assert(!IsZero(GetOpenPrice()));
	Assert(!IsZero(GetActiveQty()));
	AssertEq(INTENTION_HOLD, GetIntention());
	PriceCheckResult result = {};
	result.start = ScaledPrice(GetOpenPrice() * GetActiveQty());
	result.margin
		= result.start
		+	(ScaledPrice(GetActiveQty())
			* GetSecurity().ScalePrice(maxLossMoneyPerContract));
	result.current = ScaledPrice(GetActiveQty() * GetMarketClosePrice());
	result.isAllowed = result.current <= result.margin;
	return result;
}

ScaledPrice EmaFuturesStrategy::ShortPosition::CaclCurrentProfit() const {
	Assert(!IsZero(GetOpenPrice()));
	Assert(!IsZero(GetActiveQty()));
	AssertEq(INTENTION_HOLD, GetIntention());
	return 
		ScaledPrice(GetActiveQty() * GetMarketClosePrice())
		- ScaledPrice(GetOpenPrice() * GetActiveQty());

}

////////////////////////////////////////////////////////////////////////////////
