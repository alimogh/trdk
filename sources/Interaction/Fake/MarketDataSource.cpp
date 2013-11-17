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

namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Fake;

Fake::MarketDataSource::MarketDataSource(
			const IniFileSectionRef &) {
	//...//
}

Fake::MarketDataSource::~MarketDataSource() {
	//...//
}

void Fake::MarketDataSource::Connect(const IniFileSectionRef &) {
	m_threads.create_thread([this](){NotificationThread();});
}

void Fake::MarketDataSource::NotificationThread() {

	try {

		Security::Bar bar(
			pt::second_clock::local_time(),
			pt::seconds(5),
			Security::Bar::TRADES);
		bar.openPrice = 10;
		bar.highPrice = 15;
		bar.lowPrice = 5;
		bar.closePrice = 11;
		bar.volume = 111;
		bar.count = 45;

		for ( ; ; ) {
			
			const auto &now = pt::second_clock::local_time();
			bool isBarTime = bar.time + bar.size <= now;
			
			foreach (boost::shared_ptr<Security> s, m_securityList) {
				s->AddTrade(
					boost::get_system_time(),
					ORDER_SIDE_BUY,
					10,
					20);
				if (isBarTime) {
					s->AddBar(bar);
				}
			}

			if (isBarTime) {
				bar.time = now;
			}

			boost::this_thread::sleep(pt::milliseconds(500));
		
		}
	} catch (...) {
	
		AssertFailNoException();
		throw;
	
	}

}

boost::shared_ptr<Security> Fake::MarketDataSource::CreateSecurity(
			Context &context,
			const Symbol &symbol)
		const {
	auto result = boost::shared_ptr<Security>(new Security(context, symbol));
	const_cast<MarketDataSource *>(this)
		->m_securityList.push_back(result);
	return result;
}
