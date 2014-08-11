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

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Onyx;

namespace fix = OnixS::FIX;

namespace {

	fix::ProtocolVersion::Enum ParseFixVersion(
				const IniSectionRef &configuration,
				Context::Log &log) {

		fix::ProtocolVersion::Enum result = fix::ProtocolVersion::UNKNOWN;

		const auto &fixVerStr = configuration.ReadKey("fix_version");
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
				configuration.ReadFileSystemPath("engine_settings").string());
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

	if (m_session) {
		return;
	}

	// Assert(m_securities.empty());

	const std::string senderCompId = conf.ReadKey("sender_comp_id");
	const std::string targetCompId = conf.ReadKey("target_comp_id");

	const std::string host = conf.ReadKey("server_host");
	const auto port  = conf.ReadTypedKey<int>("server_port");

	m_log.Info(
		"Connecting to FIX Server at \"%1%:%2%\""
			" with SenderCompID \"%3%\" and TargetCompID \"%4%\"...",
		boost::make_tuple(
			boost::cref(host),
			port,
			boost::cref(senderCompId),
			boost::cref(targetCompId)));

	boost::scoped_ptr<fix::Session> session(
		new fix::Session(senderCompId, targetCompId, m_fixVersion, this));
	ConnectSession(conf, *session, host, port);
	session.swap(m_session);

	m_log.Info(
		"Connected to FIX Server at \"%1%:%2%\""
			" with SenderCompID \"%3%\" and TargetCompID \"%4%\".",
		boost::make_tuple(
			boost::cref(host),
			port,
			boost::cref(senderCompId),
			boost::cref(targetCompId)));

}

void OnixFixEngine::ConnectSession(
			const IniSectionRef &,
			fix::Session &session,
			const std::string &host,
			int port) {
	try {
		session.logonAsInitiator(host, port);
	} catch (const fix::Exception &ex) {
		m_log.Error("Failed to connect to FIX Server: \"%1%\".", ex.what());
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
			trdk::Context &,
			const trdk::Lib::Symbol &)
		const {
	throw Error("OnixFixEngine::CreateSecurity not implemented");
}

void OnixFixEngine::onInboundApplicationMsg(
			fix::Message &/*message*/,
			fix::Session *) {
    // clog << "\nIncoming application-level message:\n" << msg << endl;
}

void OnixFixEngine::onInboundSessionMsg(
			fix::Message &/*message*/,
			fix::Session *) {
    // clog << "\nIncoming session-level message:\n" << msg << endl;
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
