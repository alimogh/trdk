/**************************************************************************
 *   Created: 2012/09/16 14:48:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "LiveMarketDataSource.hpp"
#include "Core/Security.hpp"

using namespace Trader::Interaction::Fake;

LiveMarketDataSource::LiveMarketDataSource() {
	//...//
}

LiveMarketDataSource::~LiveMarketDataSource() {
	//...//
}

void LiveMarketDataSource::Connect(const IniFile &, const std::string &/*section*/) {
	//...//
}

void LiveMarketDataSource::Start() {
	//...//
}

boost::shared_ptr<Trader::Security> LiveMarketDataSource::CreateSecurity(
			boost::shared_ptr<Trader::TradeSystem> tradeSystem,
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			boost::shared_ptr<const Trader::Settings> settings,
			bool logMarketData)
		const {
	return boost::shared_ptr<Trader::Security>(
		new Security(
			tradeSystem,
			symbol,
			primaryExchange,
			exchange,
			settings,
			logMarketData));
}

boost::shared_ptr<Trader::Security> LiveMarketDataSource::CreateSecurity(
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			boost::shared_ptr<const Trader::Settings> settings,
			bool logMarketData)
		const {
	return boost::shared_ptr<Trader::Security>(
		new Security(
			symbol,
			primaryExchange,
			exchange,
			settings,
			logMarketData));
}
