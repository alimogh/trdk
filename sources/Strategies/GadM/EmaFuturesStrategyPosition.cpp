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

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Strategies::GadM;
using namespace trdk::Strategies::GadM::EmaFuturesStrategy;

namespace ids = boost::uuids;

////////////////////////////////////////////////////////////////////////////////

EmaFuturesStrategy::Position::Position()
	: m_startTime(GetStrategy().GetContext().GetCurrentTime())
	, m_intention(INTENTION_OPEN_PASSIVE)
	, m_isIntentionInAction(false)  {
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

void EmaFuturesStrategy::Position::SetIntention(Intention intention) {
	AssertNe(m_intention, intention);
	AssertNe(INTENTION_OPEN_PASSIVE, intention);
	bool isIntentionInAction = false;
	Sync(intention, isIntentionInAction);
	m_intention = intention;
	m_isIntentionInAction = isIntentionInAction;
}

void EmaFuturesStrategy::Position::Sync() {
	Sync(m_intention, m_isIntentionInAction);
}

void EmaFuturesStrategy::Position::Sync(
		Intention &intention,
		bool &isIntentionInAction) {

	static_assert(numberOfIntentions == 5, "List changed.");
	switch (intention) {

		case INTENTION_OPEN_PASSIVE:
		case INTENTION_OPEN_AGGRESIVE:
			if (IsOpened()) {
				Assert(!IsCompleted());
				Assert(isIntentionInAction);
				intention = INTENTION_HOLD;
				isIntentionInAction = false;
			} else if (HasActiveOrders()) {
				if (intention == INTENTION_OPEN_AGGRESIVE) {
					if (!isIntentionInAction) {
						CancelAllOrders();
					}
				} else {
					Assert(isIntentionInAction);
				}
			} else if (!isIntentionInAction) {
				if (!IsCanceled()) {
					throw Lib::Exception("Failed to open position");
				}
				intention = INTENTION_HOLD;
			} else {
				Open(
					intention == INTENTION_OPEN_PASSIVE
						?	GetOpenStartPrice()
						:	GetMarketOpenPrice());
				isIntentionInAction = true;
			}
			break;

		case INTENTION_HOLD:
			Assert(!isIntentionInAction);
			return;

		case INTENTION_CLOSE_PASSIVE:
		case INTENTION_CLOSE_AGGRESIVE:
			Assert(IsOpened());
			if (IsCompleted()) {
				intention = INTENTION_HOLD;
				isIntentionInAction = false;
			} else if (HasActiveOrders()) {
				if (intention == INTENTION_CLOSE_AGGRESIVE) {
					if (!isIntentionInAction) {
						CancelAllOrders();
					}
				} else {
					Assert(isIntentionInAction);
				}
			} else if (!isIntentionInAction) {
				if (!IsCanceled()) {
					throw Lib::Exception("Failed to close position");
				}
				intention = INTENTION_HOLD;
			} else {
				Close(
					CLOSE_TYPE_TAKE_PROFIT,
					intention == INTENTION_CLOSE_PASSIVE
						?	GetMarketClosePrice() //! @todo fixme
						:	GetMarketClosePrice());
				isIntentionInAction = true;
			}
			break;
		
		default:
			AssertEq(INTENTION_OPEN_PASSIVE, m_intention);
	
	}

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
		strategyTimeMeasurement)
	, trdk::LongPosition() {
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

////////////////////////////////////////////////////////////////////////////////
