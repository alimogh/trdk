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
	, m_isPassiveClose(true) {
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
	GetStrategy().GetTradingLog().Write(
		"intention\t%1%->%2%\t%3%\t%4%\t%5%",
		[&](TradingRecord &record) {
			record
				% m_intention
				% intention
				% (m_isPassiveOpen ? "true" : "false")
				% (m_isPassiveClose ? "true" : "false")
				% (m_isSent ? "true" : "false");
		});
	Sync(intention);
	m_intention = intention;
}

void EmaFuturesStrategy::Position::Sync() {
	Sync(m_intention);
}

void EmaFuturesStrategy::Position::Sync(Intention &intention) {

	static_assert(numberOfIntentions == 6, "List changed.");
	switch (intention) {

		case INTENTION_OPEN_PASSIVE:
			Assert(m_isPassiveOpen);
			if (IsOpened()) {
				Assert(m_isSent);
				intention = INTENTION_HOLD;

			} else if (HasActiveOrders()) {
				Assert(m_isSent);
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
				Assert(m_isSent);
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
				Assert(m_isSent);
				intention = INTENTION_HOLD;
				m_isSent = false;
			} else if (HasActiveOrders()) {
				Assert(m_isSent);
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
				Assert(m_isSent);
				intention = INTENTION_HOLD;
			} else if (HasActiveOrders()) {
				Assert(m_isSent);
				Assert(!HasActiveOpenOrders());
			} else if (m_isSent) {
				throw Exception(
					"Order canceled by trading system without request");
			} else {
				Close(CLOSE_TYPE_TAKE_PROFIT, GetPassiveClosePrice());
				m_isPassiveClose = true;
				m_isSent = true;
				m_startTime = GetStrategy().GetContext().GetCurrentTime();
			}
			break;

		case INTENTION_CLOSE_AGGRESIVE:
			Assert(IsOpened());
			Assert(!HasActiveOpenOrders());
			if (IsCompleted()) {
				Assert(m_isSent);
				intention = INTENTION_HOLD;
			} else if (HasActiveOrders()) {
				Assert(m_isSent);
				Assert(!HasActiveOpenOrders());
				if (m_isPassiveClose) {
					CancelAllOrders();
					m_isSent = false;
				}
			} else if (m_isSent) {
				throw Exception(
					"Order canceled by trading system without request");
			} else {
				Close(CLOSE_TYPE_TAKE_PROFIT, GetMarketClosePrice());
				m_isPassiveClose = false;
				m_isSent = true;
			}
			break;
		
		default:
			AssertEq(INTENTION_OPEN_PASSIVE, intention);
			break;
	
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

ScaledPrice EmaFuturesStrategy::LongPosition::GetPassiveClosePrice() const {
	return GetSecurity().GetAskPriceScaled();
}

bool EmaFuturesStrategy::LongPosition::IsPriceAllowed(double priceDelta) const {
	Assert(HasActiveOrders());
	//! @todo
	return HasActiveOpenOrders()
		?	(GetOpenStartPrice() + priceDelta <= GetMarketOpenPrice())
		:	(GetCloseStartPrice() - priceDelta >= GetMarketClosePrice());
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

ScaledPrice EmaFuturesStrategy::ShortPosition::GetPassiveClosePrice() const {
	return GetSecurity().GetBidPriceScaled();
}

bool EmaFuturesStrategy::ShortPosition::IsPriceAllowed(
		double priceDelta)
		const {
	Assert(HasActiveOrders());
	//! @todo
	return HasActiveOpenOrders()
		?	(GetOpenStartPrice() - priceDelta >= GetMarketOpenPrice())
		:	(GetCloseStartPrice() + priceDelta <= GetMarketClosePrice());
}

////////////////////////////////////////////////////////////////////////////////
