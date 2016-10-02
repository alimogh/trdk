/**************************************************************************
 *   Created: 2016/09/29 03:43:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"
#include "Security.hpp"
#include "Core/Context.hpp"

namespace trdk { namespace Interaction { namespace Test {

	class CsvMarketDataSource : public trdk::MarketDataSource {

	public:

		typedef trdk::MarketDataSource Base;

	public:

		CsvMarketDataSource(
				size_t index,
				Context &context,
				const std::string &tag,
				const Lib::IniSectionRef &);
		virtual ~CsvMarketDataSource();

	public:

		virtual void Connect(const trdk::Lib::IniSectionRef &);

		virtual void SubscribeToSecurities();

	protected:

		virtual trdk::Security & CreateNewSecurityObject(
				const trdk::Lib::Symbol &);

	private:

		void NotificationThread();

	private:

		std::ifstream m_source;

		boost::thread_group m_threads;
		boost::atomic_bool m_stopFlag;

		std::vector<boost::shared_ptr<Security>> m_securityList;

	};

} } }
