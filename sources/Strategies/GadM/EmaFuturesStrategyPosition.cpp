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
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Strategies::GadM;
using namespace trdk::Strategies::GadM::EmaFuturesStrategy;

namespace ids = boost::uuids;

////////////////////////////////////////////////////////////////////////////////

EmaFuturesStrategy::Position::Position()
	: m_intention(INTENTION_OPEN_PASSIVE)
	, m_isSent(false)
	, m_isPassiveOpen(true)
	, m_isPassiveClose(true)
	, m_closeType(CLOSE_TYPE_NONE)
	, m_maxProfit(0) {
	//...//
}

EmaFuturesStrategy::Position::~Position() {
	//...//
}

const EmaFuturesStrategy::Position::Time &
EmaFuturesStrategy::Position::GetStartTime() const {
	return m_startTime;
}

const Intention & EmaFuturesStrategy::Position::GetIntention() const {
	return m_intention;
}

void EmaFuturesStrategy::Position::SetIntention(
		Intention intention,
		const CloseType &closeType) {
	AssertNe(m_intention, intention);
	AssertNe(INTENTION_OPEN_PASSIVE, intention);
	GetStrategy().GetTradingLog().Write(
		"intention\t%1%->%2%\t%3%\t%4%\t%5%\t%6%",
		[&](TradingRecord &record) {
			record
				% m_intention
				% intention
				% (m_isPassiveOpen ? "true" : "false")
				% (m_isPassiveClose ? "true" : "false")
				% (m_isSent ? "true" : "false")
				% closeType;
		});
	m_closeType = closeType;
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
			"order moving is already started",
			[](TradingRecord &){});
		return;
	}
	m_isSent = false;
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
				Open(GetOpenStartPrice());
				m_isPassiveOpen = true;
				m_isSent = true;
				m_startTime = GetStrategy().GetContext().GetCurrentTime();
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
				Close(m_closeType, GetCloseStartPrice());
				m_isPassiveClose = true;
				m_isSent = true;
				m_startTime = GetStrategy().GetContext().GetCurrentTime();
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

////////////////////////////////////////////////////////////////////////////////

EmaFuturesStrategy::LongPosition::LongPosition(
		Strategy &startegy,
		const ids::uuid &operationId,
		TradeSystem &tradeSystem,
		Security &security,
		const Qty &qty,
		const Milestones &strategyTimeMeasurement)
	: trdk::Position(
		startegy,
		operationId,
		0,
		tradeSystem,
		security,
		security.GetSymbol().GetCurrency(),
		qty,
		security.GetBidPriceScaled(),
		strategyTimeMeasurement) {
	//...//
}

EmaFuturesStrategy::LongPosition::~LongPosition() {
	//...//
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
		TradeSystem &tradeSystem,
		Security &security,
		const Qty &qty,
		const Milestones &strategyTimeMeasurement)
	: trdk::Position(
		startegy,
		operationId,
		0,
		tradeSystem,
		security,
		security.GetSymbol().GetCurrency(),
		qty,
		security.GetAskPriceScaled(),
		strategyTimeMeasurement) {
	//...//
}

EmaFuturesStrategy::ShortPosition::~ShortPosition() {
	//...//
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
