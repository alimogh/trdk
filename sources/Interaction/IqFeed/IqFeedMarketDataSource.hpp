/**************************************************************************
 *   Created: 2016/11/20 20:46:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"
#include "IqFeedStream.hpp"
#include "IqFeedSettings.hpp"

namespace trdk { namespace Interaction { namespace IqFeed {

	class MarketDataSource : public trdk::MarketDataSource {

	public:

		typedef trdk::MarketDataSource Base;

	public:

		explicit MarketDataSource(
				size_t index,
				trdk::Context &,
				const std::string &instanceName,
				const Lib::IniSectionRef &);
		virtual ~MarketDataSource();

	public:

		virtual void Connect(const trdk::Lib::IniSectionRef &) override;

		virtual void SubscribeToSecurities() override;
		void ResubscribeToSecurities();
		
		void OnHistoryLoadCompleted(IqFeed::Security &);

		IqFeed::Security * FindSecurityBySymbolString(const std::string &);

		const Settings & GetSettings() const {
			return m_settings;
		}

	protected:

		virtual trdk::Security & CreateNewSecurityObject(
				const trdk::Lib::Symbol &)
				override;

	private:

		Settings m_settings;

		std::unique_ptr<Stream> m_stream;
		std::unique_ptr<History> m_history;

		boost::unordered_map<std::string, boost::shared_ptr<IqFeed::Security>>
			m_securities;

	};

} } } 
