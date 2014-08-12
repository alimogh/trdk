/**************************************************************************
 *   Created: 2014/08/12 23:52:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "CurrenexFixTradeSystem.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Onyx;

namespace fix = OnixS::FIX;

CurrenexFixTradeSystem::CurrenexFixTradeSystem(
					const IniSectionRef &conf,
					Context::Log &log)
		: m_log(log),
		m_session("trading", conf, m_log) {
	//...//
}

CurrenexFixTradeSystem::~CurrenexFixTradeSystem() {
	//...//
}

void CurrenexFixTradeSystem::Connect(const IniSectionRef &conf) {
	if (m_session.IsConnected()) {
		return;
	}
	try {
		m_session.Connect(conf, *this);
	} catch (const CurrenexFixSession::ConnectError &ex) {
		throw ConnectError(ex.what());
	} catch (const CurrenexFixSession::Error &ex) {
		throw Error(ex.what());
	}
}

OrderId CurrenexFixTradeSystem::SellAtMarketPrice(
			trdk::Security &,
			trdk::Qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexFixTradeSystem::SellAtMarketPrice not implemented");
}

OrderId CurrenexFixTradeSystem::Sell(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexFixTradeSystem::Sell not implemented");
}

OrderId CurrenexFixTradeSystem::SellAtMarketPriceWithStopPrice(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice /*stopPrice*/,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error(
		"CurrenexFixTradeSystem::SellAtMarketPriceWithStopPrice"
			" not implemented");
}

OrderId CurrenexFixTradeSystem::SellOrCancel(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexFixTradeSystem::SellOrCancel not implemented");
}

OrderId CurrenexFixTradeSystem::BuyAtMarketPrice(
			trdk::Security &,
			trdk::Qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexFixTradeSystem::BuyAtMarketPrice not implemented");
}

OrderId CurrenexFixTradeSystem::Buy(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexFixTradeSystem::Buy not implemented");
}

OrderId CurrenexFixTradeSystem::BuyAtMarketPriceWithStopPrice(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice /*stopPrice*/,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error(
		"CurrenexFixTradeSystem::BuyAtMarketPriceWithStopPrice"
			" not implemented");
}

OrderId CurrenexFixTradeSystem::BuyOrCancel(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexFixTradeSystem::BuyOrCancel not implemented");
}

void CurrenexFixTradeSystem::CancelOrder(OrderId) {
	throw Error("CurrenexFixTradeSystem::CancelOrder not implemented");
}

void CurrenexFixTradeSystem::CancelAllOrders(trdk::Security &) {
	throw Error("CurrenexFixTradeSystem::CancelAllOrders not implemented");
}

void CurrenexFixTradeSystem::onStateChange(
			fix::SessionState::Enum newState,
			fix::SessionState::Enum prevState,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogStateChange(newState, prevState, *session);
}

void CurrenexFixTradeSystem::onError(
			fix::ErrorReason::Enum reason,
			const std::string &description,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogError(reason, description, *session);
}

void CurrenexFixTradeSystem::onWarning(
			fix::WarningReason::Enum reason,
			const std::string &description,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogWarning(reason, description, *session);
}

void CurrenexFixTradeSystem::onInboundApplicationMsg(
			fix::Message &/*message*/,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	UseUnused(session);
}

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXFIXENGINE_API
TradeSystemFactoryResult CreateCurrenexTrading(
			const IniSectionRef &configuration,
			Context::Log &log) {
	TradeSystemFactoryResult result;
	boost::get<0>(result).reset(
		new CurrenexFixTradeSystem(configuration, log));
	return result;
}

////////////////////////////////////////////////////////////////////////////////
