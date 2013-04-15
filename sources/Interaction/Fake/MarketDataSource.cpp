/**************************************************************************
 *   Created: 2012/09/16 14:48:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MarketDataSource.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Fake;

Fake::MarketDataSource::MarketDataSource(
			const IniFileSectionRef &,
			Context::Log &) {
	//...//
}

Fake::MarketDataSource::~MarketDataSource() {
	//...//
}

void Fake::MarketDataSource::Connect() {
	m_threads.create_thread([this](){NotificationThread();});
}

void Fake::MarketDataSource::NotificationThread() {
	try {
		for ( ; ; ) {
			foreach (boost::shared_ptr<Security> s, m_securityList) {
				s->AddTrade(
					boost::get_system_time(),
					ORDER_SIDE_BUY,
					10,
					20);
			}
			boost::this_thread::sleep(boost::posix_time::milliseconds(500));
		}
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

boost::shared_ptr<Security> Fake::MarketDataSource::CreateSecurity(
			Context &context,
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			bool logMarketData)
		const {
	auto result = boost::shared_ptr<Security>(
		new Security(
			context,
			symbol,
			primaryExchange,
			exchange,
			logMarketData));
	const_cast<MarketDataSource *>(this)
		->m_securityList.push_back(result);
	return result;
}
