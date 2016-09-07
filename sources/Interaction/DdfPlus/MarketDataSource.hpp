/**************************************************************************
 *   Created: 2016/08/24 09:48:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Stream.hpp"
#include "History.hpp"
#include "Security.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/TradingLog.hpp"

namespace trdk { namespace Interaction { namespace DdfPlus {

	class MarketDataSource : public trdk::MarketDataSource {

	public:

		typedef trdk::MarketDataSource Base;

	public:

		MarketDataSource(
				size_t index,
				Context &context,
				const std::string &tag,
				const Lib::IniSectionRef &);
		virtual ~MarketDataSource();

	public:

		virtual void Connect(const trdk::Lib::IniSectionRef &);

		virtual void SubscribeToSecurities();

	public:

		using Base::FindSecurity;

		DdfPlus::Security * FindSecurity(
				const std::string &ddfPlusCodeSymbolCode);

	protected:

		virtual trdk::Security & CreateNewSecurityObject(
				const trdk::Lib::Symbol &symbolDdfPlusCode);

	private:

		void CheckHistoryConnection();

	private:

		std::unique_ptr<Stream> m_stream;
		std::unique_ptr<History> m_history;

		boost::unordered_map<std::string, boost::shared_ptr<DdfPlus::Security>>
			m_securities;

	};

} } }
