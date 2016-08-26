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

#include "Connection.hpp"
#include "Security.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/TradingLog.hpp"

namespace trdk { namespace Interaction { namespace DdfPlus {

	class MarketDataSource
		: public trdk::MarketDataSource,
		public ConnectionDataHandler {

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

		virtual trdk::MarketDataSource & GetSource();
		virtual const trdk::MarketDataSource & GetSource() const;

	protected:

		virtual trdk::Security & CreateNewSecurityObject(
				const trdk::Lib::Symbol &);

	private:

		Connection m_connection;

		std::vector<boost::shared_ptr<DdfPlus::Security>> m_securities;

	};

} } }
