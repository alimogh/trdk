/**************************************************************************
 *   Created: 2014/08/12 23:01:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "CurrenexSession.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace fix = OnixS::FIX;

namespace {

	fix::ProtocolVersion::Enum ParseFixVersion(
				const IniSectionRef &configuration,
				Context::Log &log,
				const std::string &sessionType) {

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
				"Failed to parse FIX Version for \"%1%\": \"%2%\"."
					" Can be: \"FIX 4.0\", \"FIX 4.1\", \"FIX 4.2\""
					", \"FIX 4.3\", \"FIX 4.4\", \"FIX 5.0\", \"FIX 5.0 SP1\""
					" or \"FIX 5.0 SP2\".",
				boost::make_tuple(sessionType, fixVerStr));
			throw CurrenexFixSession::Error("Failed to init FIX Engine");
		}
	
		Log::Debug(
			"Using FIX Version \"%1%\" (%2%) for \"%3%\".",
			boost::make_tuple(fixVerStr, result, sessionType));

		return result;

	}

}

CurrenexFixSession::CurrenexFixSession(
			const std::string &type,
			const IniSectionRef &configuration,
			Context::Log &log)
		: m_type(type),
		m_log(log),
		m_fixVersion(ParseFixVersion(configuration, m_log, m_type)) {
	
	if (!fix::Engine::initialized()) {
		const auto &settings
			= configuration.GetBase().ReadFileSystemPath(
				"Common",
				"onixs_fix_engine_settings");
		m_log.Info(
			"Initializing FIX Engine with %1%...",
			settings);
		try {
			fix::Engine::init(settings.string());
		} catch (const fix::Exception &ex) {
			m_log.Error("Failed to init FIX Engine: \"%1%\".", ex.what());
			throw Error("Failed to init FIX Engine");
		}
	} else {
		m_log.Debug("FIX Engine already initialized.");
	}
	Assert(fix::Engine::initialized());

}

CurrenexFixSession::~CurrenexFixSession() {
	Assert(fix::Engine::initialized());
}

void CurrenexFixSession::Connect(
			const IniSectionRef &conf,
			fix::ISessionListener &listener) {

	Assert(!m_session);

	const std::string senderCompId = conf.ReadKey("sender_comp_id");
	const std::string targetCompId = conf.ReadKey("target_comp_id");
	
	const std::string host = conf.ReadKey("server_host");
	const auto port = conf.ReadTypedKey<int>("server_port");

#	ifdef _DEBUG
		bool resetSeqNumFlag = true;
#	else
		bool resetSeqNumFlag = false;
#	endif
	const char *const resetSeqNumFlagSection = "Common";
	const char *const resetSeqNumFlagKey = "onixs_fix_reset_seq_num_flag";
	resetSeqNumFlag = conf.GetBase().ReadBoolKey(
		resetSeqNumFlagSection,
		resetSeqNumFlagKey,
		resetSeqNumFlag);

	m_log.Info(
		"Connecting to FIX Server (%1%) at \"%2%:%3%\""
			" with SenderCompID \"%4%\" and TargetCompID \"%5%\""
			", ResetSeqNumFlag: %6%::%7% = %8%...",
		boost::make_tuple(
			boost::cref(m_type),
			boost::cref(host),
			port,
			boost::cref(senderCompId),
			boost::cref(targetCompId),
			resetSeqNumFlagSection,
			resetSeqNumFlagKey,
			resetSeqNumFlag ? "true" : "false"));

	boost::scoped_ptr<fix::Session> session(
		new fix::Session(senderCompId, targetCompId, m_fixVersion, &listener));
	if (conf.ReadBoolKey("use_ssl")) {
		session->encryptionMethod(fix::EncryptionMethod::SSL);
	}

	fix::Message customLogonMessage("A", GetFixVersion());

	customLogonMessage.set(
		fix::FIX43::Tags::Password,
		conf.ReadKey("password"));

	// Asynchronous calls from FIX engine, must be in field before connected:
	session.swap(m_session);
	try {
		m_session->logonAsInitiator(
			host,
			port,
			30,
			&customLogonMessage,
			resetSeqNumFlag);
	} catch (const fix::Exception &ex) {
		m_session.reset();
		m_log.Error(
			"Failed to connect to FIX Server (%1%): \"%2%\".",
			boost::make_tuple(
				boost::cref(m_type),
				ex.what()));
		throw ConnectError("Failed to connect to FIX Server");
	} catch (...) {
		m_session.reset();
		throw;
	}
	
	m_log.Info("Connected to FIX Server (%1%).", m_type);

}

void CurrenexFixSession::LogStateChange(
			fix::SessionState::Enum newState,
			fix::SessionState::Enum prevState,
			fix::Session &session) {
	Assert(&session == &Get());
	UseUnused(session);
	const auto newStateStr = fix::SessionState::toString(newState);
	const auto prevStateStr = fix::SessionState::toString(prevState);
	m_log.Info(
		"FIX Session State changed: \"%1%\" -> \"%2%\".",
		boost::make_tuple(boost::cref(prevStateStr), boost::cref(newStateStr)));
}

void CurrenexFixSession::LogError(
			fix::ErrorReason::Enum reason,
			const std::string &description,
			fix::Session &session) {
	Assert(&session == &Get());
	UseUnused(session);
    m_log.Error(
		"FIX Session error: \"%1%\" (%2%)",
		boost::make_tuple(boost::cref(description), reason));
}

void CurrenexFixSession::LogWarning(
			fix::WarningReason::Enum reason,
			const std::string &description,
			fix::Session &session) {
	Assert(&session == &Get());
	UseUnused(session);
	m_log.Warn(
		"FIX Session waring: \"%1%\" (%2%)",
		boost::make_tuple(boost::cref(description), reason));
}

