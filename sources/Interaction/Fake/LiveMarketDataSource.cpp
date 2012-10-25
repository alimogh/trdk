/**************************************************************************
 *   Created: 2012/09/16 14:48:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "LiveMarketDataSource.hpp"

using namespace Trader::Interaction::Fake;

LiveMarketDataSource::LiveMarketDataSource() {
	//...//
}

LiveMarketDataSource::~LiveMarketDataSource() {
	//...//
}

void LiveMarketDataSource::Connect() {
	m_threads.create_thread([this](){NotificationThread();});
}

void LiveMarketDataSource::NotificationThread() {
	try {
		for ( ; ; ) {
			foreach (boost::shared_ptr<Security> s, m_securityList) {
				s->SignaleNewTrade(boost::get_system_time(), true, 10, 20);
			}
			boost::this_thread::sleep(boost::posix_time::milliseconds(500));
		}
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

boost::shared_ptr<Trader::Security> LiveMarketDataSource::CreateSecurity(
			boost::shared_ptr<Trader::TradeSystem> tradeSystem,
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			boost::shared_ptr<const Trader::Settings> settings,
			bool logMarketData)
		const {
	auto result = boost::shared_ptr<Security>(
		new Security(
			tradeSystem,
			symbol,
			primaryExchange,
			exchange,
			settings,
			logMarketData));
	m_securityList.push_back(result);
	return result;
}

boost::shared_ptr<Trader::Security> LiveMarketDataSource::CreateSecurity(
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			boost::shared_ptr<const Trader::Settings> settings,
			bool logMarketData)
		const {
	auto result = boost::shared_ptr<Security>(
		new Security(
			symbol,
			primaryExchange,
			exchange,
			settings,
			logMarketData));
	m_securityList.push_back(result);
	return result;
}
