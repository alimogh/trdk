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
	, m_historySize(conf.ReadTypedKey<size_t>("stat_history_size", 3))
	, m_emaSpeedSlow(conf.ReadTypedKey<double>("ema_speed_slow"))
	, m_emaSpeedFast(conf.ReadTypedKey<double>("ema_speed_fast"))
	, m_slowEmaAcc(accs::tag::ema_speed::speed = m_emaSpeedSlow)
	, m_fastEmaAcc(accs::tag::ema_speed::speed = m_emaSpeedFast) {

	m_instancies.push_back(this);

	GetLog().Info(
		"History size: %1%. Book size: %2% * 2 price levels.",
		m_historySize,
		m_bookLevelsCount);

	if (m_historySize < 2) {
		throw ModuleError("History size can be less then 2.");
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

	////////////////////////////////////////////////////////////////////////////////
	// Theo:
	
	Point point = {
		Round(
			(bidStat.avgPrice * askStat.qty + askStat.avgPrice * bidStat.qty)
				/ (bidStat.qty + askStat.qty),
		security.GetPriceScale())
	};

	////////////////////////////////////////////////////////////////////////////////
	// EMAs:
	
	// accumulates values for EMA:
	m_slowEmaAcc(point.theo);
	// gets current EMA value:
	point.emaSlow = Round(
		accs::ema(m_slowEmaAcc),
		security.GetPriceScale());

	// accumulates values for EMA:
	m_fastEmaAcc(point.theo);
	// gets current EMA value:
	point.emaFast = Round(
		accs::ema(m_fastEmaAcc),
		security.GetPriceScale());

	////////////////////////////////////////////////////////////////////////////////
	// Preparing previous 2 points for actual strategy work:
	
	while (m_statMutex.test_and_set(boost::memory_order_acquire));
	m_stat.history.emplace_back(point);
	while (m_stat.history.size() > m_historySize) {
		m_stat.history.pop_front();
	}
	m_statMutex.clear(boost::memory_order_release);

	////////////////////////////////////////////////////////////////////////////////
	
	// If this method returns true - strategy will be notified about actual data
	// update:
	AssertGe(m_historySize, m_stat.history.size());
	return
		m_stat.history.size() >= m_historySize
		&& !IsZero(m_stat.history.front().theo);

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

	const auto newHistorySize = conf.ReadTypedKey<size_t>(
		"stat_history_size",
		m_historySize);

	const auto newEmaSpeedSlow = conf.ReadTypedKey<double>("ema_speed_slow");
	const auto newEmaSpeedFast = conf.ReadTypedKey<double>("ema_speed_fast");

	bool reset = false;

	if (newHistorySize != m_historySize) {
		GetLog().Info(
			"Set new history size: %1% -> %2%.",
			m_historySize,
			newHistorySize);
		if (newHistorySize < 2) {
			GetLog().Error(
				"Failed to set new history size"
					", history size can't be less then 2.");
		} else {
			m_historySize = newHistorySize;
			reset = true;
		}
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
