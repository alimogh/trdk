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
#include "Core/PriceBook.hpp"

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

		const double bidStart = 70.00;
		const double askStart = 80.00;

		bool isCorrectionPositive = true;
		boost::mt19937 random;
		boost::uniform_int<uint16_t> topRandomRange(0, 1100);
		boost::variate_generator<boost::mt19937, boost::uniform_int<uint16_t>>
			generateTopRandom(random, topRandomRange);
		boost::uniform_int<uint8_t> stepRandomRange(0, 2);
		boost::variate_generator<boost::mt19937, boost::uniform_int<uint8_t>>
			generateStepRandom(random, stepRandomRange);

		while (!m_stopFlag) {

			const auto &now = GetContext().GetCurrentTime();

			double bid
				= bidStart
				+ ((double(generateTopRandom()) / 100)  * (isCorrectionPositive ? 1 : -1));
			double ask
				= askStart
				+ ((double(generateTopRandom()) / 100) * (isCorrectionPositive ? 1 : -1));
			isCorrectionPositive = !isCorrectionPositive;

			foreach (const auto &s, m_securityList) {

				const auto &timeMeasurement
					= GetContext().StartStrategyTimeMeasurement();

				PriceBook book(now);
				
				for (int i = 1; i <= book.GetSideMaxSize(); ++i) {
					book.GetAsk().Add(
						now,
						Round(ask, s->GetPriceScale()),
						i * ((generateTopRandom() + 1) * 10000));
					ask += (double(generateStepRandom()) / 100) + 0.01;
				}
				
				for (int i = 1; i <= book.GetSideMaxSize(); ++i) {
					book.GetBid().Add(
						now,
						Round(bid, s->GetPriceScale()),
						i * ((generateTopRandom() + 1) * 20000));
					bid -= (double(generateStepRandom()) / 100) + 0.01;
				}

				s->SetBook(book, timeMeasurement);

			}

			if (m_stopFlag) {
				break;
			}
			boost::this_thread::sleep(pt::seconds(20));
		
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
