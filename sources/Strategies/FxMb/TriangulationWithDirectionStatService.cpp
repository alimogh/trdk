/**************************************************************************
 *   Created: 2014/11/30 05:21:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TriangulationWithDirectionStatService.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Settings.hpp"
#include "Core/AsyncLog.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;
namespace accs = boost::accumulators;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::FxMb;
using namespace trdk::Strategies::FxMb::Twd;

std::vector<StatService *> StatService::m_instancies;

StatService::StatService(
		Context &context,
		const std::string &tag,
		const IniSectionRef &conf)
	: Base(context, "TriangulationWithDirectionStatService", tag)
	, m_bookLevelsCount(
	 	conf.GetBase().ReadTypedKey<size_t>("General", "book.levels.count"))
	, m_prev1Duration(
	 	pt::milliseconds(
	 		conf.ReadTypedKey<size_t>("prev1_duration_miliseconds")))
	, m_prev2Duration(
	 	pt::milliseconds(
	 		conf.ReadTypedKey<size_t>("prev2_duration_miliseconds")))
	, m_emaSpeedSlow(conf.ReadTypedKey<double>("ema_speed_slow"))
	, m_emaSpeedFast(conf.ReadTypedKey<double>("ema_speed_fast"))
	, m_slowEmaAcc(accs::tag::ema_speed::speed = m_emaSpeedSlow)
	, m_fastEmaAcc(accs::tag::ema_speed::speed = m_emaSpeedFast) {

	m_instancies.push_back(this);

	GetLog().Info(
		"Prev1 duration: %1%; Prev2 duration: %2%."
			" Book size: %3% * 2 price levels.",
		m_prev1Duration,
		m_prev2Duration,
		m_bookLevelsCount);

	if (m_prev2Duration <= m_prev1Duration) {
		throw ModuleError("Prev2 duration can't be equal or less then Prev1");
	}

	if (m_bookLevelsCount < 1) {
		throw ModuleError("Book Size can't be \"zero\"");
	}

}

StatService::~StatService() {
	try {
		const auto &pos = std::find(
			m_instancies.begin(),
			m_instancies.end(),
			this);
		Assert(pos != m_instancies.end());
		m_instancies.erase(pos);
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

pt::ptime StatService::OnSecurityStart(const Security &security) {
	const auto &dataIndex = security.GetSource().GetIndex();
	if (m_sources.size() <= dataIndex) {
		m_sources.resize(dataIndex + 1, {});
	}
	Assert(!m_sources[dataIndex].security);
	m_sources[dataIndex].security = &security;
	return pt::not_a_date_time;
}

bool StatService::OnBookUpdateTick(
		const Security &security,
		const Security::Book &book,
		const TimeMeasurement::Milestones &) {

	// Method will be called at each book update (add new price level, removed
	// price level or price level updated).

	// Store each book update as "last book" to calculate stat by each book
	// tick. But only if it has changes in top n-lines - if there are no
	// changes in top lines - skip calculation and trading.
	{
		Source &source = m_sources[security.GetSource().GetIndex()];
		Assert(source.security == &security);
		bool hasChanges = source.bidsBook.GetSize() != book.GetBids().GetSize();
		for (
			size_t i = 0;
			!hasChanges && i < std::min(m_bookLevelsCount, book.GetBids().GetSize());
			hasChanges = source.bidsBook.GetLevel(i) != book.GetBids().GetLevel(i), ++i);
		if (!hasChanges) {
			hasChanges = source.asksBook.GetSize() != book.GetAsks().GetSize();
			for (
				size_t i = 0;
				!hasChanges && i < std::min(m_bookLevelsCount, book.GetAsks().GetSize());
				hasChanges = source.asksBook.GetLevel(i) != book.GetAsks().GetLevel(i), ++i);
			if (!hasChanges) {
				return false;
			}
		}
		Assert(
			source.time == pt::not_a_date_time
			|| source.time <= book.GetTime());
		source.time = book.GetTime();
		source.bidsBook = book.GetBids();
		source.asksBook = book.GetAsks();
	}

	////////////////////////////////////////////////////////////////////////////////
	// Aggregating books, preparing opportunity statistics:

	struct PriceLevel {
		double price;
		Qty qty;
	}; 
	struct SideStat {
		Qty qty;
		double vol;
		double avgPrice;
		SideStat & operator <<(PriceLevel &rhs) {
			Assert(!IsZero(rhs.price));
			Assert(!IsZero(rhs.qty));
			qty += rhs.qty;
			vol += rhs.qty * rhs.price;
			return *this;
		}
	};
	SideStat bidStat = {};
	SideStat askStat = {};

/*#	ifdef BOOST_ENABLE_ASSERT_HANDLER
		PriceLevel prevBid = {};
		PriceLevel prevAsk = {};
#	endif*/
	for (size_t i = 0; i < m_bookLevelsCount; ++i) {

		PriceLevel bid = {};
		PriceLevel ask = {};

		foreach (const Source &source, m_sources) {

			if (source.bidsBook.GetSize() > i) {
				const auto &sourceBid = source.bidsBook.GetLevel(i);
				Assert(!IsZero(sourceBid.GetPrice()));
				Assert(!IsZero(sourceBid.GetQty()));
				if (bid.price <= sourceBid.GetPrice()) {
					if (IsEqual(bid.price, sourceBid.GetPrice())) {
						bid.qty += sourceBid.GetQty();
					} else {
						bid.price = sourceBid.GetPrice();
						bid.qty = sourceBid.GetQty();
					}
				}
			}

			if (source.asksBook.GetSize() > i) {
				const auto &sourceAsk = source.asksBook.GetLevel(i);
				Assert(!IsZero(sourceAsk.GetPrice()));
				Assert(!IsZero(sourceAsk.GetQty()));
				if (IsZero(ask.price) || ask.price > sourceAsk.GetPrice()) {
					ask.price = sourceAsk.GetPrice();
					ask.qty = sourceAsk.GetQty();
				} else if (IsEqual(ask.price, sourceAsk.GetPrice())) {
					ask.qty += sourceAsk.GetQty();
				}
			}

		}

		if (IsZero(bid.price) || IsZero(ask.price)) {
			return false;
		}

		Assert(!IsZero(bid.qty));
		Assert(!IsZero(ask.price));
/*#		ifdef BOOST_ENABLE_ASSERT_HANDLER
			if (i != 0) {
				AssertLt(prevBid.price, bid.price);
				AssertGt(prevAsk.price, ask.price);
			}
			prevBid = bid;
			prevAsk = ask;
#		endif */

		bidStat << bid;
		askStat << ask;

	}

	Assert(!IsZero(bidStat.vol));
	Assert(!IsZero(askStat.vol));
	Assert(!IsZero(bidStat.qty));
	Assert(!IsZero(askStat.qty));
	
	bidStat.avgPrice = bidStat.vol / bidStat.qty;
	askStat.avgPrice = askStat.vol / askStat.qty;

	UpdateNumberOfUpdates(book);

	Point point = {book.GetTime()};
	foreach (const Source &source, m_sources) {
		if (source.time != pt::not_a_date_time && source.time > point.time) {
			point.time = source.time;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	// Theo:
	
	point.theo = Round(
		(bidStat.avgPrice * askStat.qty + askStat.avgPrice * bidStat.qty)
			/ (bidStat.qty + askStat.qty),
		security.GetPriceScale());

	////////////////////////////////////////////////////////////////////////////////
	// EMAs:
	
	// accumulates values for EMA:
	m_slowEmaAcc(point.theo);
	// gets current EMA value:
	point.emaSlow = Round(accs::ema(m_slowEmaAcc), security.GetPriceScale());

	// accumulates values for EMA:
	m_fastEmaAcc(point.theo);
	// gets current EMA value:
	point.emaFast = Round(accs::ema(m_fastEmaAcc), security.GetPriceScale());

	////////////////////////////////////////////////////////////////////////////////
	// Preparing previous 2 points for actual strategy work:
	
	Stat stat = {m_numberOfUpdates.size(), point};
	m_statHistory.emplace_back(point);
	
	const auto &p2Time = point.time - m_prev2Duration;
	const auto &p1Time = point.time - m_prev1Duration;
	
	auto itP1 = m_statHistory.cend();
	auto itP2 = m_statHistory.cend();
	for (auto it = m_statHistory.cbegin(); it != m_statHistory.cend(); ) {
	
		if (it->time <= p2Time) {
	
			AssertLt(it->time, p1Time);
			const auto next = it + 1;
			AssertLe(it->time, next->time);
			if (next == m_statHistory.cend()) {
				// Point only one, and it was a long time ago, so this value
				// was actual at p1 and p2 too.
				itP1
					= itP2
					= it;
				break;
			}
			if (next->time <= p2Time) {
				// Need to remove first point, it already too old for us:
				Assert(it == m_statHistory.cbegin());
				m_statHistory.pop_front();
				it = m_statHistory.cbegin();
				continue;
			}
			// As this time is suitable for p2 - it can be suitable for p1 too:
			itP1
				= itP2
				= it;
		
		} else if (it->time <= p1Time) {
		
			// P2, if was, found, now searching only p1:
			Assert(itP2 == m_statHistory.cend() || itP2->time < it->time);
			itP1 = it;
		
		} else {

			// All found (what exists):
			break;
		
		}
		
		++it;
	
	}
	
	Assert(
		itP1 == m_statHistory.cend()
		|| itP2 == m_statHistory.cend()
		|| (itP2->time <= itP1->time && itP1->time < point.time));
	if (itP1 != m_statHistory.cend()) {
		stat.prev1 = *itP1;
	} else {
		// There is no history at start, and only at start:
		AssertEq(stat.prev1.time, pt::not_a_date_time);
	}
	if (itP2 != m_statHistory.cend()) {
		stat.prev2 = *itP2;
	} else {
		// There is no history at start, and only at start:
		AssertEq(stat.prev2.time, pt::not_a_date_time);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Storing new values as actual:
	
	while (m_statMutex.test_and_set(boost::memory_order_acquire));
	m_stat = stat;
	m_statMutex.clear(boost::memory_order_release);

	////////////////////////////////////////////////////////////////////////////////
	
	// If this method returns true - strategy will be notified about actual data
	// update:
	return !IsZero(stat.prev2.theo);

}

void StatService::UpdateNumberOfUpdates(const Security::Book &book) {
	const auto &startTime = book.GetTime() - pt::seconds(30);
	while (
			!m_numberOfUpdates.empty()
			&& m_numberOfUpdates.front() < startTime) {
		m_numberOfUpdates.pop_front();
	}
	m_numberOfUpdates.push_back(book.GetTime());
}

void StatService::OnSettingsUpdate(const IniSectionRef &conf) {

	Base::OnSettingsUpdate(conf);

	const auto newPrev1Duration = pt::milliseconds(
		conf.ReadTypedKey<size_t>("prev1_duration_miliseconds"));
	const auto newPrev2Duration = pt::milliseconds(
		conf.ReadTypedKey<size_t>("prev2_duration_miliseconds"));

	const auto newEmaSpeedSlow = conf.ReadTypedKey<double>("ema_speed_slow");
	const auto newEmaSpeedFast = conf.ReadTypedKey<double>("ema_speed_fast");

	bool reset = false;

	if (
			m_prev1Duration != newPrev1Duration
			|| m_prev2Duration != newPrev2Duration) {
		GetLog().Info(
			"Set new periods: Prev1 %1% -> %2%; Prev2 %3% -> %4%.",
			m_prev1Duration,
			newPrev1Duration,
			m_prev2Duration,
			newPrev2Duration);
		m_prev1Duration = newPrev1Duration;
		m_prev2Duration = newPrev2Duration;
		reset = true;
	}

	if (m_emaSpeedSlow != m_emaSpeedSlow || m_emaSpeedFast != newEmaSpeedFast) {
		GetLog().Info(
			"Set EMA speed: Slow %1% -> %2%; Fast %3% -> %4%.",
			m_emaSpeedSlow,
			newEmaSpeedSlow,
			m_emaSpeedFast,
			newEmaSpeedFast);
		m_emaSpeedSlow = newEmaSpeedSlow;
		m_emaSpeedFast = newEmaSpeedFast;
		reset = true;
	}


	if (reset) {

		while (m_statMutex.test_and_set(boost::memory_order_acquire));
		m_stat = {};
		m_statMutex.clear(boost::memory_order_release);

		m_slowEmaAcc = EmaAcc(accs::tag::ema_speed::speed = m_emaSpeedSlow);
		m_fastEmaAcc = EmaAcc(accs::tag::ema_speed::speed = m_emaSpeedFast);

		m_statHistory.clear();

	}

}

////////////////////////////////////////////////////////////////////////////////

TRDK_STRATEGY_FXMB_API
boost::shared_ptr<Service> CreateTriangulationWithDirectionStatService(
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	return boost::shared_ptr<Service>(
		new Twd::StatService(context, tag, configuration));
}

////////////////////////////////////////////////////////////////////////////////
