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
				ModuleEventsLog &log) {

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
				"Failed to parse FIX version: \"%1%\"."
					" Can be: \"FIX 4.0\", \"FIX 4.1\", \"FIX 4.2\""
					", \"FIX 4.3\", \"FIX 4.4\", \"FIX 5.0\", \"FIX 5.0 SP1\""
					" or \"FIX 5.0 SP2\".",
				fixVerStr);
			throw FixSession::Error("Failed to init FIX Engine");
		}
	
		log.Debug("Using FIX version \"%1%\" (%2%).", fixVerStr, result);

		return result;

	}

}

namespace {
	boost::atomic_size_t fixEngineRefCounter(0);
}

FixSession::FixSession(
			Context &context,
			ModuleEventsLog &log,
			const IniSectionRef &configuration)
		: m_context(context),
		m_log(log),
		m_fixVersion(ParseFixVersion(configuration, m_log)),
		m_isSessionActive(false) {

	if (!fix::Engine::initialized()) {
		AssertEq(0, fixEngineRefCounter.load());
		const auto &settings
			= configuration.GetBase().ReadFileSystemPath(
				"General",
				"onixs_fix_engine_settings");
		m_log.Info("Initializing FIX Engine with %1%...", settings);
		try {
			fix::Engine::init(settings.string());
		} catch (const fix::Exception &ex) {
			m_log.Error("Failed to init FIX Engine: \"%1%\".", ex.what());
			throw Error("Failed to init FIX Engine");
		}
	} else {
		AssertLt(0, fixEngineRefCounter.load());
		m_log.Debug(
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
	m_log.Debug("Session destroyed.");
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

	const char *const resetSeqNumFlagSection = "General";
	const char *const resetSeqNumFlagKey = "onixs_fix_reset_seq_num_flag";
	const bool isLocalResetSeqNumFlagKey = conf.IsKeyExist(resetSeqNumFlagKey);
	bool resetSeqNumFlag = isLocalResetSeqNumFlagKey
		?	conf.ReadBoolKey(resetSeqNumFlagKey)
		:	conf.GetBase().ReadBoolKey(
				resetSeqNumFlagSection,
				resetSeqNumFlagKey);

	m_log.Info(
		"Connecting to server at \"%1%:%2%\""
			" with SenderCompID \"%3%\" and TargetCompID \"%4%\""
			", ResetSeqNumFlag: %5%::%6% = %7%...",
		m_host,
		m_port,
		senderCompId,
		targetCompId,
		(isLocalResetSeqNumFlagKey ? conf.GetName() : resetSeqNumFlagSection),
		resetSeqNumFlagKey,
		resetSeqNumFlag ? "true" : "false");

#	ifdef DEV_VER
		conf.GetBase().ReadBoolKey("General", "onixs_fix_session_log");
		const auto &sessionStorageType = fix::SessionStorageType::FileBased;
#	else
		const auto &sessionStorageType
			= conf.GetBase().ReadBoolKey("General", "onixs_fix_session_log")
				?	fix::SessionStorageType::FileBased
				:	fix::SessionStorageType::MemoryBased;
#	endif

	boost::scoped_ptr<fix::Session> session(
		new fix::Session(
			senderCompId,
			targetCompId,
			m_fixVersion,
			&listener,
			sessionStorageType));

	session->encryptionMethod(
		fix::EncryptionMethod::Enum(conf.ReadTypedKey<int>("encrypt_method")));

	if (!senderSubId.empty()) {
		session->senderSubId(senderSubId);
	}

	m_customLogonMessage = fix::Message("A", GetFixVersion());

	const std::string username = conf.ReadKey("Username", std::string());
	if (!username.empty()) {
		m_customLogonMessage.set(fix::FIX43::Tags::Username, username);
	}

	if (conf.IsKeyExist("password")) {
		m_customLogonMessage.set(
			fix::FIX43::Tags::Password,
			conf.ReadKey("password"));
	}

	// Asynchronous calls from FIX engine, must be in field before connected:
	session.swap(m_session);

	m_log.Debug(
		"In-sequence number: %1%; Out-sequence number: %2%;",
		m_session->inSeqNum(),
		m_session->outSeqNum());

	try {
		m_session->logonAsInitiator(
			m_host,
			m_port,
			30,
			&m_customLogonMessage,
			resetSeqNumFlag);
		m_isSessionActive = true;
	} catch (const fix::Exception &ex) {
		m_log.Error("Failed to connect to server: \"%1%\".", ex.what());
		throw ConnectError("Failed to connect to server");
	}
	
	m_log.Info("Connected to server.");

}

void FixSession::Reconnect() {
	if (!m_isSessionActive) {
		m_log.Debug("No connection exists, reconnecting canceled.");
		return;
	}
	m_log.Info("Reconnecting to server at \"%1%:%2%\"...", m_host, m_port);
 	m_session->logonAsInitiator(m_host, m_port);
 	m_log.Info("Reconnected to server.");
}

void FixSession::Disconnect() {

	m_isSessionActive = false;
	if (!m_session) {
		return;
	}

	if (m_session->state() == fix::SessionState::Enum::Active) {
		m_log.Debug("Sending logout...");
		m_session->logout("Logout");
		m_log.Debug("Logout sent.");
	} else {
		m_log.Debug("Session not active - no logout.");
	}

	m_log.Debug("Session shutdown...");
	m_session->shutdown();
	m_log.Debug("Waiting for FIX Engine threads stop (workaround)...");
	boost::this_thread::sleep(boost::posix_time::seconds(4));

}

void FixSession::LogStateChange(
		fix::SessionState::Enum newState,
		fix::SessionState::Enum prevState,
		fix::Session &session) {
	Assert(&session == &Get());
	UseUnused(session);
	const auto newStateStr = fix::SessionState::toString(newState);
	const auto prevStateStr = fix::SessionState::toString(prevState);
	const char *message = "FIX session state changed: \"%1%\" -> \"%2%\".";
	if (newState == fix::SessionState::Disconnected) {
		m_log.Warn(message, prevStateStr, newStateStr);
	} else {
		m_log.Info(message, prevStateStr, newStateStr);
	}
}

void FixSession::LogError(
			fix::ErrorReason::Enum reason,
			const std::string &description,
			fix::Session &session) {
	Assert(&session == &Get());
	UseUnused(session);
    m_log.Error("%1% (%2%).", description, reason);
}

void FixSession::LogWarning(
			fix::WarningReason::Enum reason,
			const std::string &description,
			fix::Session &session) {
	Assert(&session == &Get());
	UseUnused(session);
	m_log.Warn("%1% (%2%)", description, reason);
}

void FixSession::ResetLocalSequenceNumbers(bool in, bool out) {
	if (in) {
		m_log.Info(
			"In-sequence number will be reset %1%...",
			m_session->inSeqNum());
 		m_session->inSeqNum(1);
	}
	if (out) {
		m_log.Info(
			"Out-sequence number will be reset from %1%...",
			m_session->outSeqNum());
 		m_session->outSeqNum(1);
	}
}
