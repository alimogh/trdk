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
#include "CurrenexTrading.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace fix = OnixS::FIX;

CurrenexTrading::CurrenexTrading(
					const IniSectionRef &conf,
					Context::Log &log)
		: m_log(log),
		m_session("trading", conf, m_log) {
	//...//
}

CurrenexTrading::~CurrenexTrading() {
	//...//
}

void CurrenexTrading::Connect(const IniSectionRef &conf) {
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

OrderId CurrenexTrading::SellAtMarketPrice(
			trdk::Security &,
			trdk::Qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexTrading::SellAtMarketPrice not implemented");
}

OrderId CurrenexTrading::Sell(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexTrading::Sell not implemented");
}

OrderId CurrenexTrading::SellAtMarketPriceWithStopPrice(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice /*stopPrice*/,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error(
		"CurrenexTrading::SellAtMarketPriceWithStopPrice"
			" not implemented");
}

OrderId CurrenexTrading::SellOrCancel(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexTrading::SellOrCancel not implemented");
}

OrderId CurrenexTrading::BuyAtMarketPrice(
			trdk::Security &,
			trdk::Qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexTrading::BuyAtMarketPrice not implemented");
}

OrderId CurrenexTrading::Buy(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexTrading::Buy not implemented");
}

OrderId CurrenexTrading::BuyAtMarketPriceWithStopPrice(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice /*stopPrice*/,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error(
		"CurrenexTrading::BuyAtMarketPriceWithStopPrice"
			" not implemented");
}

OrderId CurrenexTrading::BuyOrCancel(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexTrading::BuyOrCancel not implemented");
}

void CurrenexTrading::CancelOrder(OrderId) {
	throw Error("CurrenexTrading::CancelOrder not implemented");
}

void CurrenexTrading::CancelAllOrders(trdk::Security &) {
	throw Error("CurrenexTrading::CancelAllOrders not implemented");
}

void CurrenexTrading::onStateChange(
			fix::SessionState::Enum newState,
			fix::SessionState::Enum prevState,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogStateChange(newState, prevState, *session);
}

void CurrenexTrading::onError(
			fix::ErrorReason::Enum reason,
			const std::string &description,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogError(reason, description, *session);
}

void CurrenexTrading::onWarning(
			fix::WarningReason::Enum reason,
			const std::string &description,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	m_session.LogWarning(reason, description, *session);
}

void CurrenexTrading::onInboundApplicationMsg(
			fix::Message &/*message*/,
			fix::Session *session) {
	Assert(session == &m_session.Get());
	UseUnused(session);
}

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXSFIXCONNECTOR_API
TradeSystemFactoryResult CreateCurrenexTrading(
			const IniSectionRef &configuration,
			Context::Log &log) {
	TradeSystemFactoryResult result;
	boost::get<0>(result).reset(
		new CurrenexTrading(configuration, log));
	return result;
}

////////////////////////////////////////////////////////////////////////////////
