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

		const double bid = 12.99;
		const double ask = 13.99;
		int correction = 1;

		while (!m_stopFlag) {

			const auto &now = GetContext().GetCurrentTime();

			foreach (const auto &s, m_securityList) {

				const auto &timeMeasurement
					= GetContext().StartStrategyTimeMeasurement();

				Security::BookUpdateOperation book
					= s->StartBookUpdate(now, false);
				{
					std::vector<Security::Book::Level> asks;
					asks.reserve(10);
					for (int i = 1; i <= 10; ++i) {
						asks.push_back(
							Security::Book::Level(
								now,
								ask + abs(correction * i),
								i * 10000));
					}
					book.GetAsks().Swap(asks);
				}
				{
					std::vector<Security::Book::Level> bids;
					bids.reserve(10);
					for (int i = 1; i <= 10; ++i) {
						bids.push_back(
							Security::Book::Level(
								now,
								bid - abs(correction * i),
								i * 10000));
					}
					book.GetBids().Swap(bids);
				}
				book.Commit(timeMeasurement);

			}

			correction *= -1;

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
	auto result = boost::make_shared<FakeSecurity>(GetContext(), symbol, *this);
	const_cast<MarketDataSource *>(this)->m_securityList.push_back(result);
	return *result;
}
