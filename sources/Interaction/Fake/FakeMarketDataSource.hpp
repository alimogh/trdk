/**************************************************************************
 *   Created: 2012/09/11 18:52:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"
#include "FakeSecurity.hpp"
#include "Core/Context.hpp"

namespace trdk { namespace Interaction { namespace Fake {

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

		virtual void SubscribeToSecurities() {
			//...//
		}

	protected:

		virtual trdk::Security & CreateNewSecurityObject(
				const trdk::Lib::Symbol &);

	private:

		void NotificationThread();

	private:

		boost::thread_group m_threads;
		boost::atomic_bool m_stopFlag;

		std::vector<boost::shared_ptr<FakeSecurity>> m_securityList;

	};

} } }
