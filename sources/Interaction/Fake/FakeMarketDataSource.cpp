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
#include "FakeMarketDataSource.hpp"
#include "Core/TradingLog.hpp"

namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Fake;

Fake::MarketDataSource::MarketDataSource(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &)
	: Base(index, context, tag),
	m_stopFlag(false) {
	//...//
}

Fake::MarketDataSource::~MarketDataSource() {
	m_stopFlag = true;
	m_threads.join_all();
	// Each object, that implements CreateNewSecurityObject should waite for
	// log flushing before destroying objects:
	GetTradingLog().WaitForFlush();
}

void Fake::MarketDataSource::Connect(const IniSectionRef &) {
	if (m_threads.size()) {
		return;
	}
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

		const double bid = 12.99;
		const double ask = 13.99;					
		int correction = 1;

		while (!m_stopFlag) {

		
			const auto &now = pt::second_clock::local_time();
			bool isBarTime = bar.time + bar.size <= now;

			foreach (boost::shared_ptr<Security> s, m_securityList) {
				const auto &timeMeasurement
					= GetContext().StartStrategyTimeMeasurement();
 				s->AddTrade(
 					GetContext().GetCurrentTime(),
 					ORDER_SIDE_BUY,
 					10,
 					20,
 					timeMeasurement);
 				if (isBarTime) {
 					s->AddBar(bar);
 				}
				s->SetLevel1(
					bid + correction,
					10000,
					ask + correction,
					10000,
					timeMeasurement);
			}

			correction *= -1;

			if (isBarTime) {
				bar.time = now;
			}

			if (m_stopFlag) {
				break;
			}
			boost::this_thread::sleep(pt::milliseconds(1));
		
		}
	} catch (...) {
	
		AssertFailNoException();
		throw;
	
	}

}

Security & Fake::MarketDataSource::CreateNewSecurityObject(
		const Symbol &symbol) {
	auto result = boost::shared_ptr<Security>(
		new Security(GetContext(), symbol, *this));
	const_cast<MarketDataSource *>(this)
		->m_securityList.push_back(result);
	return *result;
}
