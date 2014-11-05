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
#include "FixSession.hpp"

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
				sessionType,
				fixVerStr);
			throw FixSession::Error("Failed to init FIX Engine");
		}
	
		Log::Debug(
			"Using FIX Version \"%1%\" (%2%) for \"%3%\".",
			fixVerStr,
			result,
			sessionType);

		return result;

	}

}

namespace {
	boost::atomic_size_t fixEngineRefCounter(0);
}

FixSession::FixSession(
			Context &context,
			const std::string &type,
			const IniSectionRef &configuration)
		: m_context(context),
		m_type(type),
		m_fixVersion(ParseFixVersion(configuration, GetLog(), m_type)),
		m_isSessionActive(false) {
	
	if (!fix::Engine::initialized()) {
		AssertEq(0, fixEngineRefCounter.load());
		const auto &settings
			= configuration.GetBase().ReadFileSystemPath(
				"Common",
				"onixs_fix_engine_settings");
		GetLog().Info(
			"Initializing FIX Engine with %1%...",
			settings);
		try {
			fix::Engine::init(settings.string());
		} catch (const fix::Exception &ex) {
			GetLog().Error("Failed to init FIX Engine: \"%1%\".", ex.what());
			throw Error("Failed to init FIX Engine");
		}
	} else {
		AssertLt(0, fixEngineRefCounter.load());
		GetLog().Debug(
			"FIX Engine already initialized (%1%).",
			fixEngineRefCounter.load());
	}
	++fixEngineRefCounter;
	Assert(fix::Engine::initialized());

}

FixSession::~FixSession() {
	Assert(fix::Engine::initialized());
	AssertLt(0, fixEngineRefCounter.load());
	if (!--fixEngineRefCounter) {
		try {
			fix::Engine::shutdown();
		} catch (...) {
			AssertFailNoException();
		}
	}
}

void FixSession::Connect(
			const IniSectionRef &conf,
			fix::ISessionListener &listener) {

	Assert(!m_session);

	const std::string senderCompId = conf.ReadKey("sender_comp_id");
	const std::string senderSubId
		= conf.ReadKey("sender_sub_id", std::string());
	const std::string targetCompId = conf.ReadKey("target_comp_id");
	
	m_host = conf.ReadKey("server_host");
	m_port = conf.ReadTypedKey<int>("server_port");

	const char *const resetSeqNumFlagSection = "Common";
	const char *const resetSeqNumFlagKey = "onixs_fix_reset_seq_num_flag";
	const bool resetSeqNumFlag = conf.GetBase().ReadBoolKey(
		resetSeqNumFlagSection,
		resetSeqNumFlagKey);

	GetLog().Info(
		"Connecting to FIX Server (%1%) at \"%2%:%3%\""
			" with SenderCompID \"%4%\" and TargetCompID \"%5%\""
			", ResetSeqNumFlag: %6%::%7% = %8%...",
		m_type,
		m_host,
		m_port,
		senderCompId,
		targetCompId,
		resetSeqNumFlagSection,
		resetSeqNumFlagKey,
		resetSeqNumFlag ? "true" : "false");

#	ifdef _DEBUG
		const auto &sessionStorageType = fix::SessionStorageType::FileBased;
#	else
		const auto &sessionStorageType = fix::SessionStorageType::MemoryBased;
#	endif

	boost::scoped_ptr<fix::Session> session(
		new fix::Session(
			senderCompId,
			targetCompId,
			m_fixVersion,
			&listener,
			sessionStorageType));
	if (conf.ReadBoolKey("use_ssl")) {
		session->encryptionMethod(fix::EncryptionMethod::SSL);
	}
	if (!senderSubId.empty()) {
		session->senderSubId(senderSubId);
	}

	m_customLogonMessage = fix::Message("A", GetFixVersion());

	const std::string username = conf.ReadKey("Username", std::string());
	if (!username.empty()) {
		m_customLogonMessage.set(
			fix::FIX43::Tags::Username,
			username);
	}

	m_customLogonMessage.set(
		fix::FIX43::Tags::Password,
		conf.ReadKey("password"));

	// Asynchronous calls from FIX engine, must be in field before connected:
	session.swap(m_session);
	try {
		m_session->logonAsInitiator(
			m_host,
			m_port,
			30,
			&m_customLogonMessage,
			resetSeqNumFlag);
		m_isSessionActive = true;
	} catch (const fix::Exception &ex) {
		GetLog().Error(
			"Failed to connect to FIX Server (%1%): \"%2%\".",
			m_type,
			ex.what());
		throw ConnectError("Failed to connect to FIX Server");
	}
	
	GetLog().Info("Connected to FIX Server (%1%).", m_type);

}

void FixSession::Reconnect() {
	if (!m_isSessionActive) {
		GetLog().Debug("No connection exists, reconnecting canceled.");
		return;
	}
	GetLog().Info(
		"Reconnecting to FIX Server (%1%) at \"%2%:%3%\"...",
		m_type,
		m_host,
		m_port);
 	m_session->logonAsInitiator(m_host, m_port);
 	GetLog().Info("Reconnected to FIX Server (%1%).", m_type);
}

void FixSession::Disconnect() {
	m_isSessionActive = false;
	if (!m_session) {
		return;
	}
	m_session->logout("Logout");
	m_session->shutdown();
}

void FixSession::LogStateChange(
			fix::SessionState::Enum newState,
			fix::SessionState::Enum prevState,
			fix::Session &session) {
	Assert(&session == &Get());
	UseUnused(session);
	const auto newStateStr = fix::SessionState::toString(newState);
	const auto prevStateStr = fix::SessionState::toString(prevState);
	const char *message = "FIX Session State changed: \"%1%\" -> \"%2%\".";
	if (newState == fix::SessionState::Disconnected) {
		GetLog().Error(message, prevStateStr, newStateStr);
	} else {
		GetLog().Info(message, prevStateStr, newStateStr);
	}
}

void FixSession::LogError(
			fix::ErrorReason::Enum reason,
			const std::string &description,
			fix::Session &session) {
	Assert(&session == &Get());
	UseUnused(session);
    GetLog().Error("FIX Session error: \"%1%\" (%2%)", description, reason);
}

void FixSession::LogWarning(
			fix::WarningReason::Enum reason,
			const std::string &description,
			fix::Session &session) {
	Assert(&session == &Get());
	UseUnused(session);
	GetLog().Warn("FIX Session waring: \"%1%\" (%2%)", description, reason);
}

void FixSession::ResetLocalSequenceNumbers() {
	GetLog().Info(
		"FIX Session:"
			" sequence number will be reset from out %1% and in %2%...",
		m_session->outSeqNum(),
		m_session->inSeqNum());
	// m_session->resetLocalSequenceNumbers();
 	m_session->outSeqNum(1);
 	m_session->inSeqNum(1);
}
