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
#include "Core/Security.hpp"

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	//! FIX price stream connection with OnixS C++ FIX Engine.
	class FixStream
			: public trdk::MarketDataSource,
			public OnixS::FIX::ISessionListener {

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

		size_t GetLevelsCount() const {
			return m_levelsCount;
		}

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
		virtual void onWarning (
					OnixS::FIX::WarningReason::Enum,
					const std::string &description,
					OnixS::FIX::Session *);

	protected:

		virtual trdk::Security & CreateSecurity(const trdk::Lib::Symbol &);

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

		virtual void OnLogout() = 0;
		virtual void OnReconnecting() = 0;

		virtual void SetupBookRequest(OnixS::FIX::Message &) const = 0;

	private:

		bool AdjustBook(
				const FixSecurity &security,
				std::vector<Security::Book::Level> &bids,
				std::vector<Security::Book::Level> asks,
				const OnixS::FIX::Message &)
				const;

	private:

		FixSession m_session;
		std::vector<boost::shared_ptr<FixSecurity>> m_securities;

		bool m_isSubscribed;

		const size_t m_levelsCount;

	};

} } }
