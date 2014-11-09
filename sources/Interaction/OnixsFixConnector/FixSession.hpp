/**************************************************************************
 *   Created: 2014/08/12 22:37:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Interactor.hpp"
#include "Core/Context.hpp"

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	class FixSession : private boost::noncopyable {

	public:

		typedef trdk::Interactor::Error Error;
		typedef trdk::Interactor::ConnectError ConnectError;

	public:

		explicit FixSession(
					Context &,
					ModuleEventsLog &,
					const Lib::IniSectionRef &);
		~FixSession();

	public:

		const OnixS::FIX::ProtocolVersion::Enum GetFixVersion() const {
			return m_fixVersion;
		}

		bool IsConnected() const {
			return m_session ? true : false;
		}

		OnixS::FIX::Session & Get() {
			Assert(IsConnected());
			return *m_session;
		}

	public:

		//! Makes serve connection or throws an exception.
		/** @throw Error
		  * @throw ConnectError
		  */
		void Connect(
						const trdk::Lib::IniSectionRef &,
						OnixS::FIX::ISessionListener &);
		void Reconnect();
		void Disconnect();

	public:

		void LogStateChange(
						OnixS::FIX::SessionState::Enum newState,
						OnixS::FIX::SessionState::Enum prevState,
						OnixS::FIX::Session &);
		void LogError(
						OnixS::FIX::ErrorReason::Enum,
						const std::string &description,
						OnixS::FIX::Session &);
		void LogWarning(
						OnixS::FIX::WarningReason::Enum,
						const std::string &description,
						OnixS::FIX::Session &);

	public:

		void ResetLocalSequenceNumbers();

	private:

		Context &m_context;

		ModuleEventsLog &m_log;

		const OnixS::FIX::ProtocolVersion::Enum m_fixVersion;

		boost::scoped_ptr<OnixS::FIX::Session> m_session;
		OnixS::FIX::Message m_customLogonMessage;
		std::string m_host;
		int m_port;
		boost::atomic_bool m_isSessionActive;

	};

} } }
