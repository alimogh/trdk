/**************************************************************************
 *   Created: 2012/08/28 20:38:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"

namespace Trader {  namespace Interaction { namespace Enyx {

	class FeedHandler;
	class Security;

} } }

namespace Trader {  namespace Interaction { namespace Enyx {

	class MarketDataSource : public Trader::LiveMarketDataSource {

	public:

		MarketDataSource(const IniFile &, const std::string &section);
		virtual ~MarketDataSource();

	public:

		virtual void Connect();

	public:

		virtual boost::shared_ptr<Trader::Security> CreateSecurity(
					boost::shared_ptr<Trader::TradeSystem>,
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					boost::shared_ptr<const Trader::Settings>,
					bool logMarketData)
				const;
		virtual boost::shared_ptr<Trader::Security> CreateSecurity(
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					boost::shared_ptr<const Trader::Settings>,
					bool logMarketData)
				const;

	protected:

		bool IsSupported(const Trader::Security &) const;
		void CheckSupport(const Trader::Security &) const;

		void CheckState() const;

		void Subscribe(const boost::shared_ptr<Security> &security) const;

	private:

		std::unique_ptr<EnyxMDInterface> m_enyx;
		std::unique_ptr<FeedHandler> m_handler;

	};

} } }
