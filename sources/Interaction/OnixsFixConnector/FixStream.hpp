/**************************************************************************
 *   Created: 2014/08/12 22:28:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "FixSession.hpp"
#include "Core/MarketDataSource.hpp"
#include "FixSecurity.hpp"

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	//! FIX price stream connection with OnixS C++ FIX Engine.
	class FixStream
			: public trdk::MarketDataSource,
			public OnixS::FIX::ISessionListener {

	private:

		typedef boost::mutex StateMutex;
		typedef StateMutex::scoped_lock StateLock;

	public:

		using Interactor::Error;

	public:

		explicit FixStream(
					size_t index,
					Context &,
					const std::string &tag,
					const Lib::IniSectionRef &);
		virtual ~FixStream();

	public:

		virtual void Connect(const trdk::Lib::IniSectionRef &);

		virtual void SubscribeToSecurities();

	public:

		virtual void onInboundApplicationMsg(
					OnixS::FIX::Message &,
					OnixS::FIX::Session *);
		virtual void onStateChange(
					OnixS::FIX::SessionState::Enum newState,
					OnixS::FIX::SessionState::Enum prevState,
					OnixS::FIX::Session *);
		virtual void onError (
					OnixS::FIX::ErrorReason::Enum,
					const std::string &description,
					OnixS::FIX::Session *session);
		virtual void onWarning(
					OnixS::FIX::WarningReason::Enum,
					const std::string &description,
					OnixS::FIX::Session *);

	protected:

		virtual trdk::Security & CreateNewSecurityObject(
				const trdk::Lib::Symbol &);

		FixSession & GetSession() {
			return m_session;
		}

	protected:

		FixSecurity * FindRequestSecurity(const OnixS::FIX::Message &);
		const FixSecurity * FindRequestSecurity(
					const OnixS::FIX::Message &)
					const;
		
		std::string GetRequestSymbolStr(const OnixS::FIX::Message &) const;

		virtual Qty ParseMdEntrySize(const OnixS::FIX::GroupInstance &) const;
		virtual Qty ParseMdEntrySize(const OnixS::FIX::Message &) const;

		virtual void OnMarketDataSnapshot(
					const OnixS::FIX::Message &,
					const boost::posix_time::ptime &dataTime,
					FixSecurity &security);

		virtual void OnLogout() = 0;
		virtual void OnReconnecting() = 0;

		virtual void SetupBookRequest(
				OnixS::FIX::Message &,
				const Security &)
				const
			= 0;

	private:

		FixSession m_session;
		std::vector<boost::shared_ptr<FixSecurity>> m_securities;

		bool m_isSubscribed;

		StateMutex m_stateMutex;
		OnixS::FIX::SessionState::Enum m_state;

	};

} } }
