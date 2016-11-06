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

const char * EmaFuturesStrategy::ConvertToPch(const Intention &intention) {
	static_assert(numberOfIntentions == 6, "List changed.");
	switch (intention) {
		default:
			AssertEq(INTENTION_OPEN_PASSIVE, intention);
			return "UNKNOWN";
		case INTENTION_OPEN_PASSIVE:
			return "open-passive";
		case INTENTION_OPEN_AGGRESIVE:
			return "open-aggresive";
		case INTENTION_HOLD:
			return "hold";
		case INTENTION_DONOT_OPEN:
			return "donot-open";
		case INTENTION_CLOSE_PASSIVE:
			return "close-passive";
		case INTENTION_CLOSE_AGGRESIVE:
			return "close-aggresive";
	}
}

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

const pt::ptime & EmaFuturesStrategy::Position::GetStartTime() const {
	return m_startTime;
}

const pt::ptime & EmaFuturesStrategy::Position::GetCloseStartTime() const {
	Assert(m_closeStartTime != pt::not_a_date_time);
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
				% ConvertToPch(m_intention)
				% ConvertToPch(intention)
				% (m_isPassiveOpen ? "true" : "false")
				% (m_isPassiveClose ? "true" : "false")
				% (m_isSent ? "true" : "false")
				% closeType
				% ConvertToPch(closeReason);
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

void EmaFuturesStrategy::Position::SetIntention(
		Intention intention,
		const CloseType &type,
		const Direction &closeReason,
		const Qty &intentionSize) {
	Assert(!m_intentionSize);
	m_intentionSize = std::make_pair(GetActiveQty(), intentionSize);
	GetStrategy().GetTradingLog().Write(
		"intention\tpartial\t%1% - %2%",
		[this](TradingRecord &record) {
			record % m_intentionSize->first % m_intentionSize->second;
		});
	SetIntention(intention, type, closeReason);
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
				record
					% ConvertToPch(m_intention)
					% m_closeType
					% ConvertToPch(m_reasons[1]);
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
					% ConvertToPch(m_intention)
					% m_closeType
					% ConvertToPch(m_reasons[1]);
			});
			break;
		case INTENTION_CLOSE_PASSIVE:
			m_intention = INTENTION_CLOSE_AGGRESIVE;
			GetStrategy().GetTradingLog().Write(
				"move\t%1%->%2%\t%3%\t%4%",
				[&](TradingRecord &record) {
					record
						% INTENTION_CLOSE_PASSIVE
						% ConvertToPch(m_intention)
						% m_closeType
						% ConvertToPch(m_reasons[1]);
				});
			break;
		default:
			GetStrategy().GetTradingLog().Write(
				"move\t%1%\t%2%\t%3%",
				[&](TradingRecord &record) {
					record
						% ConvertToPch(m_intention)
						% m_closeType
						% ConvertToPch(m_reasons[1]);
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
						GetStrategy(),
						GetId(),
						startTime);
				});
				Open(GetOpenStartPrice());
				m_isPassiveOpen = true;
				m_isSent = true;
				m_startTime = std::move(startTime);
			}
			break;

		case INTENTION_DONOT_OPEN:
			if (HasActiveOpenOrders()) {
				CancelAllOrders();
				m_isSent = false;
			} else if (m_isSent) {
				throw Exception(
					"Order canceled by trading system without request");
			} else {
				// Some position was opened by active order immediately after
				// INTENTION_DONOT_OPEN intension set.
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
				if (!m_intentionSize) {
					throw Exception(
						"Order canceled by trading system without request");
				}
				m_isSent = false;
				if (m_intentionSize->first > GetActiveQty()) {
					m_intentionSize = boost::none;
					m_isPassiveClose = true;
					intention = INTENTION_HOLD;
				} else {
					Sync(intention);
				}
			} else {

				SetCloseStartPrice(GetMarketCloseOppositePrice());
				m_closeStartTime = GetStrategy().GetContext().GetCurrentTime();

				m_signalsBidAsk[1] = std::make_pair(
					GetSecurity().GetBidPrice(),
					GetSecurity().GetAskPrice());

				m_signalsEmas[1] = std::make_pair(
					m_emas[SLOW].GetValue(),
					m_emas[FAST].GetValue());

				if (!m_intentionSize) {
					Close(m_closeType, GetCloseStartPrice());
				} else {
					Close(
						m_closeType,
						GetCloseStartPrice(),
						m_intentionSize->second);
				}
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
				Assert(m_closeStartTime != pt::not_a_date_time);
				if (!m_intentionSize) {
					throw Exception(
						"Order canceled by trading system without request");
				}
				m_isSent = false;
				if (m_intentionSize->first > GetActiveQty()) {
					m_intentionSize = boost::none;
					m_isPassiveClose = true;
					intention = INTENTION_HOLD;
				} else {
					Sync(intention);
				}
			} else {
				SetCloseStartPrice(GetMarketClosePrice());
				if (m_closeStartTime.is_not_a_date_time()) {
					m_closeStartTime
						= GetStrategy().GetContext().GetCurrentTime();
				}
				if (!m_intentionSize) {
					Close(m_closeType, GetCloseStartPrice());
				} else {
					Close(
						m_closeType,
						GetCloseStartPrice(),
						m_intentionSize->second);
				}
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
		double trailingRatio)
		const {
	PriceCheckResult result = {};
	result.current = GetSecurity().ScalePrice(GetPlannedPnl());
	if (result.current > m_maxProfit) {
		const_cast<ScaledPrice &>(m_maxProfit) = result.current;
	}
	result.start = m_maxProfit;
	result.margin = m_maxProfit - ScaledPrice(m_maxProfit * trailingRatio);
	const ScaledPrice minProfitVol
		= GetSecurity().ScalePrice(minProfit) * ScaledPrice(GetActiveQty());
	result.isAllowed
		= m_maxProfit < minProfitVol || result.current > result.margin;
	return result;
}

Qty EmaFuturesStrategy::Position::CheckProfitLevel(
		const ProfitLevels &profitLevels)
		const {

	AssertLt(0, profitLevels.priceStep);
	AssertLt(0, profitLevels.numberOfContractsPerLevel.size());

	const auto minChange = GetSecurity().ScalePrice(profitLevels.priceStep);
	const auto currentChange = IsLong()
		?	GetMarketClosePrice() - GetLastTradePrice()
		:	GetLastTradePrice() - GetMarketClosePrice();
	if (currentChange < minChange) {
		return 0;
	}
	size_t numberOfSteps = size_t(currentChange / minChange);
	AssertLt(0, numberOfSteps);

	Qty closedLevels = 0;
	Qty result = 0;
	for (auto levelIt = profitLevels.numberOfContractsPerLevel.cbegin(); ; ) {
		const auto nextLevelSize
			=	levelIt != profitLevels.numberOfContractsPerLevel.cend()
				?	*levelIt
				:	profitLevels.numberOfContractsPerLevel.back();
		if (GetClosedQty() > closedLevels) {
			closedLevels += nextLevelSize;
			if (levelIt != profitLevels.numberOfContractsPerLevel.cend()) {
				++levelIt;
			}
		} else {
			result += nextLevelSize;
			if (!--numberOfSteps) {
				break;
			}
		}
	}

	GetStrategy().GetTradingLog().Write(
		"slow-order\tprofit-level"
			"\tcurrent=%1$.2f\tmin=%2$.2f\torder_qty=%3%"
			"\tlast_price=%4$.2f\tclosed_qty=%5%\tactive_qty=%6%"
			"\tbid=%7$.2f\task=%8$.2f",
		[&](TradingRecord &record) {
			record
				% GetSecurity().DescalePrice(currentChange)
				% GetSecurity().DescalePrice(minChange)
				% result
				% GetSecurity().DescalePrice(GetLastTradePrice())
				% GetClosedQty()
				% GetActiveQty()
				% GetSecurity().GetBidPrice()
				% GetSecurity().GetAskPrice();
			});

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
		/* 9	*/ << ",P&L vol."
		/* 10	*/ << ",P&L %"
		/* 11	*/ << ",Is Profit"
		/* 12	*/ << ",Is Loss"
		/* 13	*/ << ",Qty"
		/* 14	*/ << ",Entry Reason"
		/* 15	*/ << ",Entry Price"
		/* 16	*/ << ",Entry Volume"
		/* 17	*/ << ",Entry Tades"
		/* 18	*/ << ",Entry Sig. Bid"
		/* 19	*/ << ",Entry Sig. Ask"
		/* 20	*/ << ",Entry Sig. Slow EMA"
		/* 21	*/ << ",Entry Sig. Fast EMA"
		/* 22	*/ << ",Exit Reason"
		/* 23	*/ << ",Exit Price"
		/* 24	*/ << ",Exit Volume"
		/* 25	*/ << ",Exit Trades"
		/* 26	*/ << ",Exit Sig. Bid"
		/* 27	*/ << ",Exit Sig. Ask"
		/* 28	*/ << ",Exit Sig. Slow EMA"
		/* 29	*/ << ",Exit Sig. Fast EMA"
		/* 30	*/ << ",ID"
		<< std::endl;
}

void EmaFuturesStrategy::Position::Report() throw() {

	if (!GetOpenedQty() || !IsCompleted()) {
		return;
	}

	try {
		GetStrategy().GetContext().InvokeDropCopy(
			[this](DropCopy &dropCopy) {
				dropCopy.ReportOperationEnd(
					GetId(),
					GetCloseTime(),
					!IsProfit()
						? OPERATION_RESULT_LOSS
						: OPERATION_RESULT_PROFIT,
					GetRealizedPnlRatio(),
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

		// 10. pnl vol.:
		m_reportStream << ',' << GetRealizedPnl();

		// 10. pnl %:
		m_reportStream << ',' << GetRealizedPnlPercentage();

		// 11. is profit, 12. is loss: 
		m_reportStream << (IsProfit() ? ",1,0" : ",0,1");

		// 13. qty:
		m_reportStream << ',' << GetOpenedQty();

		// 14. entry reason:
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

		// 15. entry price, 16. entry volume
		m_reportStream
			<< ',' << GetSecurity().DescalePrice(GetOpenAvgPrice())
			<< ',' << GetOpenedVolume();

		// 17. entry trades:
		m_reportStream << ',' << GetNumberOfOpenTrades();
		
		// 18. entry bid, 19. entry ask,
		m_reportStream
			<< ',' << m_signalsBidAsk[0].first
			<< ',' << m_signalsBidAsk[0].second;

		// 20. entry slow ema, 21. entry fast ema
		m_reportStream
			<< ',' << m_signalsEmas[0].first
			<< ',' << m_signalsEmas[0].second;

		// 22. exit reason:
		m_reportStream << ',';
		if (m_closeType != CLOSE_TYPE_NONE) {
			m_reportStream << m_closeType;
		} else {
			switch (m_reasons[1]) {
				default:
					AssertEq(DIRECTION_UP, m_reasons[1]);
				case DIRECTION_LEVEL:
					m_reportStream << "UNKNOWN";
					break;
				case DIRECTION_UP:
					m_reportStream << "fast EMA grows";
					break;
				case DIRECTION_DOWN:
					m_reportStream << "fast EMA falls";
					break;
			}
		}

		// 23. exit price, 24. exit volume,
		m_reportStream
			<< ',' << GetSecurity().DescalePrice(GetCloseAvgPrice())
			<< ',' << GetClosedVolume();

		// 25. exit trades:
		m_reportStream << ',' << GetNumberOfCloseTrades();

		// 26. exit bid, 27. exit ask,
		m_reportStream
			<< ',' << m_signalsBidAsk[1].first
			<< ',' << m_signalsBidAsk[1].second;
	
		// 28. exit slow ema, 29. exit fast ema.
		m_reportStream
			<< ',' << m_signalsEmas[1].first
			<< ',' << m_signalsEmas[1].second;

		// 30. ID
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

EmaFuturesStrategy::LongPosition::PriceCheckResult
EmaFuturesStrategy::LongPosition::CheckOrderPrice(double priceDelta) const {
	Assert(HasActiveOrders());
	PriceCheckResult result = {};
	if (HasActiveOpenOrders()) {
		result.start = GetOpenStartPrice();
		result.margin = result.start + GetSecurity().ScalePrice(priceDelta);
		result.current = GetIntention() == INTENTION_OPEN_PASSIVE
			?	GetMarketOpenOppositePrice()
			:	GetMarketOpenPrice();
		result.isAllowed = result.margin >= result.current;
	} else {
		result.start = GetCloseStartPrice();
		result.margin = result.start - GetSecurity().ScalePrice(priceDelta);
		result.current = GetIntention() == INTENTION_CLOSE_PASSIVE
			?	GetMarketCloseOppositePrice()
			:	GetMarketClosePrice();
		result.isAllowed = result.margin <= result.current;
	}
	Assert(!IsZero(result.start));
	Assert(!IsZero(result.margin));
	Assert(!IsZero(result.current));
	return result;
}

EmaFuturesStrategy::LongPosition::PriceCheckResult
EmaFuturesStrategy::LongPosition::CheckStopLoss(
		double maxLossMoneyPerContract)
		const {
	Assert(!IsZero(GetOpenAvgPrice()));
	AssertEq(INTENTION_HOLD, GetIntention());
	PriceCheckResult result = {};
	result.start = ScaledPrice(GetOpenAvgPrice() * GetActiveQty());
	result.margin
		= result.start
		- (ScaledPrice(GetActiveQty())
			* GetSecurity().ScalePrice(maxLossMoneyPerContract));
	result.current = ScaledPrice(GetActiveQty() * GetMarketClosePrice());
	result.isAllowed = result.current >= result.margin;
	return result;
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
			?	GetMarketOpenOppositePrice()
			:	GetMarketOpenPrice();
		result.isAllowed = result.margin <= result.current;
	} else {
		result.start = GetCloseStartPrice();
		result.margin = result.start + GetSecurity().ScalePrice(priceDelta);
		result.current = GetIntention() == INTENTION_CLOSE_PASSIVE
			?	GetMarketCloseOppositePrice()
			:	GetMarketClosePrice();
		result.isAllowed = result.margin >= result.current;
	}
	Assert(!IsZero(result.start));
	Assert(!IsZero(result.margin));
	Assert(!IsZero(result.current));
	return result;
}

EmaFuturesStrategy::ShortPosition::PriceCheckResult
EmaFuturesStrategy::ShortPosition::CheckStopLoss(
		double maxLossMoneyPerContract)
		const {
	Assert(!IsZero(GetOpenAvgPrice()));
	Assert(!IsZero(GetActiveQty()));
	AssertEq(INTENTION_HOLD, GetIntention());
	PriceCheckResult result = {};
	result.start = ScaledPrice(GetOpenAvgPrice() * GetActiveQty());
	result.margin
		= result.start
		+	(ScaledPrice(GetActiveQty())
			* GetSecurity().ScalePrice(maxLossMoneyPerContract));
	result.current = ScaledPrice(GetActiveQty() * GetMarketClosePrice());
	result.isAllowed = result.current <= result.margin;
	return result;
}

////////////////////////////////////////////////////////////////////////////////
