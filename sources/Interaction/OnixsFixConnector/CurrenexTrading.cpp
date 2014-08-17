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
#include "Core/Security.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace fix = OnixS::FIX;

CurrenexTrading::CurrenexTrading(
					const IniSectionRef &conf,
					Context::Log &log)
		: m_log(log),
		m_session("trading", conf, m_log),
		m_nextOrderId(0) {
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

fix::Message CurrenexTrading::CreateOrderMessage(
			const OrderId &orderId,	
			const Security &security,
			const Currency &currency,
			const Qty &qty) {
	// Creates order FIX-message and sets common fields.
	fix::Message result("D", m_session.GetFixVersion());
	result.set(
		fix::FIX40::Tags::ClOrdID,
		boost::lexical_cast<std::string>(orderId));
	result.set(
		fix::FIX40::Tags::HandlInst,
		fix::FIX40::Values::HandlInst::Automated_execution_order_private_no_Broker_intervention);
	result.set(fix::FIX40::Tags::Currency, ConvertToIso(currency));
	result.set(fix::FIX40::Tags::Symbol, security.GetSymbol().GetSymbol());
	result.set(
		fix::FIX40::Tags::TransactTime,
		fix::Timestamp::utc(),
		fix::TimestampFormat::YYYYMMDDHHMMSSMsec);
	result.set(fix::FIX40::Tags::OrderQty, qty);
	return std::move(result);
}

OrderId CurrenexTrading::SellAtMarketPrice(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			trdk::Qty qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	const auto &orderId = TakeOrderId();
	fix::Message order = CreateOrderMessage(orderId, security, currency, qty);
	order.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Buy);
	order.set(
		fix::FIX40::Tags::OrdType,
		fix::FIX41::Values::OrdType::Forex_Market);
	m_session.Get().send(&order);
	return orderId;
}

OrderId CurrenexTrading::Sell(
			trdk::Security &,
			const trdk::Lib::Currency &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexTrading::Sell not implemented");
}

OrderId CurrenexTrading::SellAtMarketPriceWithStopPrice(
			trdk::Security &,
			const trdk::Lib::Currency &,
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
			const trdk::Lib::Currency &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexTrading::SellOrCancel not implemented");
}

OrderId CurrenexTrading::BuyAtMarketPrice(
			trdk::Security &security,
			const trdk::Lib::Currency &currency,
			trdk::Qty qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	const auto &orderId = TakeOrderId();
	fix::Message order = CreateOrderMessage(orderId, security, currency, qty);
	order.set(fix::FIX40::Tags::Side, fix::FIX40::Values::Side::Buy);
	order.set(
		fix::FIX40::Tags::OrdType,
		fix::FIX41::Values::OrdType::Forex_Market);
	m_session.Get().send(&order);
	return orderId;
}

OrderId CurrenexTrading::Buy(
			trdk::Security &,
			const trdk::Lib::Currency &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("CurrenexTrading::Buy not implemented");
}

OrderId CurrenexTrading::BuyAtMarketPriceWithStopPrice(
			trdk::Security &,
			const trdk::Lib::Currency &,
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
			const trdk::Lib::Currency &,
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
			fix::Message &message,
			fix::Session *session) {
	
	Assert(session == &m_session.Get());
	UseUnused(session);

	if (message.type() != "8") {
		return;
	}

	const auto &execTransType = message.get(fix::FIX40::Tags::ExecTransType);

	if (execTransType == fix::FIX40::Values::ExecTransType::New) {
		
		const auto &execType = message.get(fix::FIX41::Tags::ExecType);
		const auto &ordStatus = message.get(fix::FIX40::Tags::OrdStatus);
		
		if (execType == fix::FIX41::Values::ExecType::New) {
			
			if (ordStatus == fix::FIX40::Values::OrdStatus::New) {
				OnOrderNew(message);
				return;
			}

		} else if (execType == fix::FIX41::Values::ExecType::Fill) {

			if (ordStatus == fix::FIX40::Values::OrdStatus::Partially_filled) {
				OnOrderFill(message);
				return;
			} else if (ordStatus == fix::FIX40::Values::OrdStatus::Filled) {
				OnOrderPartialFill(message);
				return;
			}

		} else if (execType == fix::FIX41::Values::ExecType::Suspended) {
			
			//...//
		
		} else if (execType == fix::FIX41::Values::ExecType::Rejected) {
			OnOrderRejected(message);
			return;
		}
		
	} else if (execTransType == fix::FIX40::Values::ExecTransType::Status) {
		
		//...//

	}

	m_log.Error(
		"FIX Server sent unknown Execution Report: \"%1%\".",
		boost::make_tuple(boost::cref(message)));

}

void CurrenexTrading::OnOrderNew(const fix::Message &/*report*/) {
	m_log.Debug("NEW!");
}		

void CurrenexTrading::OnOrderRejected(const fix::Message &reject) {
	const std::string &clOrdID = reject.get(fix::FIX40::Tags::ClOrdID);
	const std::string &reason = reject.get(fix::FIX40::Tags::Text);
	m_log.Error(
		"FIX Server Rejected order %1%: \"%2%\"."
			" Original reject: \"%3%\".",
		boost::make_tuple(
			boost::cref(clOrdID),
			boost::cref(reason),
			boost::cref(reject)));
	//! @todo remove order by clOrdID from active list here
}

void CurrenexTrading::OnOrderFill(const fix::Message &/*report*/) {
	m_log.Debug("FILLED!");
}
		
void CurrenexTrading::OnOrderPartialFill(const fix::Message &/*report*/) {
	m_log.Debug("PARTIAL FILLED!");
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
