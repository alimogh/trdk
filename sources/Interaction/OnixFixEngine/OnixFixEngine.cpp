/**************************************************************************
 *   Created: 2014/08/10 11:32:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "OnixFixEngine.hpp"
#include "Core/Security.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Onyx;

namespace fix = OnixS::FIX;

namespace {

	fix::ProtocolVersion::Enum ParseFixVersion(
				const IniSectionRef &configuration,
				Context::Log &log) {

		fix::ProtocolVersion::Enum result = fix::ProtocolVersion::UNKNOWN;

		const auto &fixVerStr = configuration.ReadKey("engine.fix_version");
		if (boost::iequals(fixVerStr, std::string("FIX 4.0"))) {
			result = fix::ProtocolVersion::FIX_40;
		} else if (boost::iequals(fixVerStr, "FIX 4.1")) {
			result = fix::ProtocolVersion::FIX_41;
		} else if (boost::iequals(fixVerStr, "FIX 4.2")) {
			result = fix::ProtocolVersion::FIX_42;
		} else if (boost::iequals(fixVerStr, "FIX 4.3")) {
			result = fix::ProtocolVersion::FIX_43;
		} else if (boost::iequals(fixVerStr, "FIX 4.4")) {
			result = fix::ProtocolVersion::FIX_44;
		} else if (boost::iequals(fixVerStr, "FIX 5.0")) {
			result = fix::ProtocolVersion::FIX_50;
		} else if (boost::iequals(fixVerStr, "FIX 5.0 SP1")) {
			result = fix::ProtocolVersion::FIX_50_SP1;
		} else if (boost::iequals(fixVerStr, "FIX 5.0 SP2")) {
			result = fix::ProtocolVersion::FIX_50_SP2;
		} else {
			log.Error(
				"Failed to parse FIX Version: \"%1%\"."
					" Can be: \"FIX 4.0\", \"FIX 4.1\", \"FIX 4.2\", \"FIX 4.3\""
					" , \"FIX 4.4\", \"FIX 5.0\", \"FIX 5.0 SP1\""
					" or \"FIX 5.0 SP2\".",
				fixVerStr);
			throw OnixFixEngine::Error("Failed to init FIX Engine");
		}
	
		Log::Debug(
			"Using FIX Version \"%1%\" (%2%).",
			boost::make_tuple(fixVerStr, result));

		return result;

	}

}


OnixFixEngine::OnixFixEngine(
			const IniSectionRef &configuration,
			Context::Log &log)
		: m_log(log),
		m_fixVersion(ParseFixVersion(configuration, m_log)) {

	if (!fix::Engine::initialized()) {
		m_log.Info("Initializing FIX Engine...");
		try {
			fix::Engine::init(
				configuration.ReadFileSystemPath("engine.settings").string());
		} catch (const fix::Exception &ex) {
			m_log.Error("Failed to init FIX Engine: \"%1%\".", ex.what());
			throw Error("Failed to init FIX Engine");
		}
	} else {
		m_log.Debug("FIX Engine already initialized..");
	}
	Assert(fix::Engine::initialized());

}

OnixFixEngine::~OnixFixEngine() {
	Assert(fix::Engine::initialized());
}

void OnixFixEngine::Connect(const trdk::Lib::IniSectionRef &conf) {

	boost::scoped_ptr<fix::Session> tradeSession;
	boost::scoped_ptr<fix::Session> streamSession;

	if (!tradeSession) {
		// Makes connection to trade-port:
		tradeSession.reset(CreateSession(conf, "trade"));
	}

	if (!m_streamSession) {
		// Makes connection to stream-port:
		streamSession.reset(CreateSession(conf, "stream"));
		SubscribeToMarketData(*streamSession);
	}

	if (tradeSession) {
		tradeSession.swap(m_tradeSession);
	}

	if (streamSession) {
		streamSession.swap(m_streamSession);
	}

}

fix::Session * OnixFixEngine::CreateSession(
			const trdk::Lib::IniSectionRef &conf,
			const std::string &prefix) {

	const std::string senderCompId = conf.ReadKey(prefix + ".sender_comp_id");
	const std::string targetCompId = conf.ReadKey(prefix + ".target_comp_id");

	const std::string host = conf.ReadKey(prefix + ".server_host");
	const auto port  = conf.ReadTypedKey<int>(prefix + ".server_port");

	m_log.Info(
		"Connecting to FIX Server (%1%) at \"%2%:%3%\""
			" with SenderCompID \"%4%\" and TargetCompID \"%5%\"...",
		boost::make_tuple(
			boost::cref(prefix),
			boost::cref(host),
			port,
			boost::cref(senderCompId),
			boost::cref(targetCompId)));

	std::unique_ptr<fix::Session> result(
		new fix::Session(senderCompId, targetCompId, m_fixVersion, this));
	ConnectSession(conf, *result, host, port, prefix);

	m_log.Info(
		"Connecting to FIX Server (%1%) at \"%2%:%3%\""
			" with SenderCompID \"%4%\" and TargetCompID \"%5%\"...",
		boost::make_tuple(
			boost::cref(prefix),
			boost::cref(host),
			port,
			boost::cref(senderCompId),
			boost::cref(targetCompId)));

	return result.release();

}

void OnixFixEngine::SubscribeToMarketData(fix::Session &session) {
	
	// Requests market data from stream by selected securities:
	foreach (const auto &security, m_securities) {
			
		const Symbol &symbol = security->GetSymbol();
		
		fix::Message mdRequest("V", GetFixVersion());

		mdRequest.set(
			fix::FIX40::Tags::Symbol,
			symbol.GetSymbol() + "/" + symbol.GetCurrency());
			
		mdRequest.set(
			fix::FIX42::Tags::SubscriptionRequestType,
			fix::FIX42::Values::SubscriptionRequestType::Snapshot__plus__Updates);
		mdRequest.set(
			fix::FIX42::Tags::MarketDepth,
			fix::FIX42::Values::MarketDepth::Top_of_Book);
		mdRequest.set(
			fix::FIX42::Tags::MDUpdateType,
			fix::FIX42::Values::MDUpdateType::Incremental_Refresh);

		auto mdEntryTypes
			= mdRequest.setGroup(fix::FIX42::Tags::NoMDEntryTypes, 2);
		mdEntryTypes[0].set(
			fix::FIX42::Tags::MDEntryType,
			fix::FIX42::Values::MDEntryType::Bid);
		mdEntryTypes[1].set(
			fix::FIX42::Tags::MDEntryType,
			fix::FIX42::Values::MDEntryType::Offer);
		
		m_log.Info(
			"Sending Market Data Request for %1%...",
			boost::cref(symbol));
		session.send(&mdRequest);

	}

}

void OnixFixEngine::ConnectSession(
			const IniSectionRef &,
			fix::Session &session,
			const std::string &host,
			int port,
			const std::string &prefix) {
	try {
		session.logonAsInitiator(host, port);
	} catch (const fix::Exception &ex) {
		GetLog().Error(
			"Failed to connect to FIX Server (%1%): \"%2%\".",
			boost::make_tuple(
				boost::cref(prefix),
				ex.what()));
		throw Error("Failed to connect to FIX Server");
	}
}

void OnixFixEngine::SubscribeToSecurities() {
	throw Error("OnixFixEngine::SubscribeToSecurities not implemented");
}

OrderId OnixFixEngine::SellAtMarketPrice(
			trdk::Security &,
			trdk::Qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("OnixFixEngine::SellAtMarketPrice not implemented");
}

OrderId OnixFixEngine::Sell(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("OnixFixEngine::Sell not implemented");
}

OrderId OnixFixEngine::SellAtMarketPriceWithStopPrice(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice /*stopPrice*/,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("OnixFixEngine::SellAtMarketPriceWithStopPrice not implemented");
}

OrderId OnixFixEngine::SellOrCancel(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("OnixFixEngine::SellOrCancel not implemented");
}

OrderId OnixFixEngine::BuyAtMarketPrice(
			trdk::Security &,
			trdk::Qty,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("OnixFixEngine::BuyAtMarketPrice not implemented");
}

OrderId OnixFixEngine::Buy(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("OnixFixEngine::Buy not implemented");
}

OrderId OnixFixEngine::BuyAtMarketPriceWithStopPrice(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice /*stopPrice*/,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("OnixFixEngine::BuyAtMarketPriceWithStopPrice not implemented");
}

OrderId OnixFixEngine::BuyOrCancel(
			trdk::Security &,
			trdk::Qty,
			trdk::ScaledPrice,
			const trdk::OrderParams &,
			const OrderStatusUpdateSlot &) {
	throw Error("OnixFixEngine::BuyOrCancel not implemented");
}

void OnixFixEngine::CancelOrder(OrderId) {
	throw Error("OnixFixEngine::CancelOrder not implemented");
}

void OnixFixEngine::CancelAllOrders(trdk::Security &) {
	throw Error("OnixFixEngine::CancelAllOrders not implemented");
}

boost::shared_ptr<trdk::Security> OnixFixEngine::CreateSecurity(
			trdk::Context &context,
			const trdk::Lib::Symbol &symbol)
		const {
	boost::shared_ptr<Security> result(new Security(context, symbol));
	const_cast<OnixFixEngine *>(this)->m_securities.push_back(result);
	return result;
}

void OnixFixEngine::onStateChange(
			fix::SessionState::Enum newState,
			fix::SessionState::Enum prevState,
			fix::Session *) {
	const auto newStateStr = fix::SessionState::toString(newState);
	const auto prevStateStr = fix::SessionState::toString(prevState);
	m_log.Info(
		"FIX Session State changed: \"%1%\" -> \"%2%\".",
		boost::make_tuple(boost::cref(prevStateStr), boost::cref(newStateStr)));
}

void OnixFixEngine::onError(
			fix::ErrorReason::Enum reason,
			const std::string &description,
			fix::Session *) {
    m_log.Error(
		"FIX Session error: \"%1%\" (%2%)",
		boost::make_tuple(boost::cref(description), reason));
}

void OnixFixEngine::onWarning(
			fix::WarningReason::Enum reason,
			const std::string &description,
			fix::Session *) {
	m_log.Warn(
		"FIX Session waring: \"%1%\" (%2%)",
		boost::make_tuple(boost::cref(description), reason));
}
