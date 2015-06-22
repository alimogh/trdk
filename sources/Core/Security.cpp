/**************************************************************************
 *   Created: 2012/07/09 18:42:14
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Security.hpp"
#include "RiskControl.hpp"
#include "MarketDataSource.hpp"
#include "Position.hpp"
#include "Settings.hpp"
#include "Context.hpp"
#include "TradingLog.hpp"

namespace fs = boost::filesystem;
namespace lt = boost::local_time;
namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;

////////////////////////////////////////////////////////////////////////////////

namespace {
	
	typedef intmax_t Level1Value;

	typedef std::array<boost::atomic<Level1Value>, numberOfLevel1TickTypes> Level1;
	
	bool IsSet(const boost::atomic<Level1Value> &val) {
		return val != std::numeric_limits<Level1Value>::max();
	}

	template<Level1TickType tick>
	typename Level1TickValuePolicy<tick>::ValueType
	GetIfSet(
				const Level1 &level1) {
		if (!IsSet(level1[tick])) {
			return 0;
		}
		return static_cast<typename Level1TickValuePolicy<tick>::ValueType>(
			level1[tick]);
	}

	void Unset(boost::atomic<Level1Value> &val) {
		val = std::numeric_limits<Level1Value>::max();
	}

	uint8_t GetPrecision(const Symbol &symbol) {
		if (boost::iequals(symbol.GetSymbol(), "EUR/USD")) {
			return 5;
		} else if (boost::iequals(symbol.GetSymbol(), "EUR/JPY")) {
			return 3;
		} else if (boost::iequals(symbol.GetSymbol(), "USD/JPY")) {
			return 3;
		} else {
			return 6;
		}
	}

}

////////////////////////////////////////////////////////////////////////////////

Security::Bar::Bar(
			const pt::ptime &time,
			const pt::time_duration &size,
			Type type)
		: time(time),
		size(size),
		type(type) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

class Security::Implementation : private boost::noncopyable {

public:

	const MarketDataSource &m_source;

	boost::array<
			boost::shared_ptr<RiskControlSymbolContext>, numberOfTradingModes>
		m_riskControlContext;

	const uint8_t m_pricePrecision;
	const uintmax_t m_priceScale;

	mutable boost::signals2::signal<Level1UpdateSlotSignature>
		m_level1UpdateSignal;
	mutable boost::signals2::signal<Level1TickSlotSignature> m_level1TickSignal;
	mutable boost::signals2::signal<NewTradeSlotSignature> m_tradeSignal;
	mutable boost::signals2::signal<BrokerPositionUpdateSlotSignature>
		m_brokerPositionUpdateSignal;
	mutable boost::signals2::signal<NewBarSlotSignature> m_barSignal;
	mutable boost::signals2::signal<BookUpdateTickSlotSignature>
		m_bookUpdateTickSignal;

	Level1 m_level1;
	boost::atomic<Qty> m_brokerPosition;
	boost::atomic_int64_t m_marketDataTime;
	boost::atomic_bool m_isLevel1Started;

	pt::ptime m_requestedDataStartTime;

	boost::shared_ptr<Book> m_book;
	boost::atomic_bool m_isBookAdjusted;

public:

	Implementation(const MarketDataSource &source, const Symbol &symbol)
		: m_source(source),
		m_pricePrecision(GetPrecision(symbol)),
		m_priceScale(size_t(std::pow(10, m_pricePrecision))),
		m_brokerPosition(0),
		m_marketDataTime(0),
		m_isLevel1Started(false),
		m_book(new Book(pt::not_a_date_time, false)),
		m_isBookAdjusted(false) {
		for (size_t i = 0; i < m_riskControlContext.size(); ++i) {
			m_riskControlContext[i]
				= m_source.GetContext()
					.GetRiskControl(TradingMode(i))
					.CreateSymbolContext(symbol);
		}
		foreach (auto &item, m_level1) {
			Unset(item);
		}
	}

	bool AddLevel1Tick(
				const pt::ptime &time,
				const Level1TickValue &tick,
				const TimeMeasurement::Milestones &timeMeasurement,
				bool flush,
				bool isPreviouslyChanged) {
		const bool isChanged = SetLevel1(
			time,
			tick,
			timeMeasurement,
			flush,
			isPreviouslyChanged);
		m_level1TickSignal(time, tick, flush);
		return isChanged;
	}

	bool SetLevel1(
				const pt::ptime &time,
				const Level1TickValue &tick,
				const TimeMeasurement::Milestones &timeMeasurement,
				bool flush,
				bool isPreviouslyChanged) {
		const auto tickValue = tick.Get();
		const bool isChanged
			= m_level1[tick.type].exchange(tickValue) != tickValue;
		FlushLevel1Update(
			time,
			timeMeasurement,
			flush,
			isChanged,
			isPreviouslyChanged);
		return isChanged;
	}

	bool CompareAndSetLevel1(
				const pt::ptime &time,
				const Level1TickValue &tick,
				intmax_t prevValue,
				const TimeMeasurement::Milestones &timeMeasurement,
				bool flush,
				bool isPreviouslyChanged) {
		const auto tickValue = tick.Get();
		AssertNe(tickValue, prevValue);
		if (tickValue == prevValue) {
			return true;
		}
		if (!m_level1[tick.type].compare_exchange_weak(prevValue, tickValue)) {
			return false;
		}
		FlushLevel1Update(
			time,
			timeMeasurement,
			flush,
			true,
			isPreviouslyChanged);
		return true;
	}

	void FlushLevel1Update(
				const pt::ptime &time,
				const TimeMeasurement::Milestones &timeMeasurement,
				bool flush,
				bool isChanged,
				bool isPreviouslyChanged) {
		
		if (!flush || !(isChanged || isPreviouslyChanged)) {
			return;
		}
		
		if (!m_isLevel1Started) {
			foreach (const auto &item, m_level1) {
				if (!IsSet(item)) {
					return;
				}
			}
			Assert(!m_isLevel1Started);
			m_isLevel1Started = true;
		}

		Assert(!time.is_not_a_date_time());
		AssertLe(GetLastMarketDataTime(), time);
		m_marketDataTime = ConvertToMicroseconds(time);

		m_level1UpdateSignal(timeMeasurement);

	}

	pt::ptime GetLastMarketDataTime() const {
		const pt::ptime result = ConvertToPTimeFromMicroseconds(m_marketDataTime);
		Assert(!result.is_not_a_date_time());
		return result;
	}

};

//////////////////////////////////////////////////////////////////////////

Security::Security(
		Context &context,
		const Symbol &symbol,
		const MarketDataSource &source)
	: Base(context, symbol),
	m_pimpl(new Implementation(source, GetSymbol())) {
}

Security::~Security() {
	delete m_pimpl;
}

RiskControlSymbolContext & Security::GetRiskControlContext(
		const TradingMode &mode) {
	return *m_pimpl->m_riskControlContext[mode];
}

const MarketDataSource & Security::GetSource() const {
	return m_pimpl->m_source;
}

uintmax_t Security::GetPriceScale() const {
	return m_pimpl->m_priceScale;
}

uint8_t Security::GetPricePrecision() const throw() {
	return m_pimpl->m_pricePrecision;
}

ScaledPrice Security::ScalePrice(double price) const {
	return ScaledPrice(Scale(price, GetPriceScale()));
}

double Security::DescalePrice(ScaledPrice price) const {
	return Descale(price, GetPriceScale());
}
double Security::DescalePrice(double price) const {
	return Descale(price, GetPriceScale());
}

pt::ptime Security::GetLastMarketDataTime() const {
	return m_pimpl->GetLastMarketDataTime();
}

bool Security::IsLevel1Started() const {
	return m_pimpl->m_isLevel1Started;
}

void Security::StartLevel1() {
	Assert(!m_pimpl->m_isLevel1Started);
	m_pimpl->m_isLevel1Started = true;
}

bool Security::IsStarted() const {
	return IsLevel1Started();
}

void Security::SetRequestedDataStartTime(const pt::ptime &newTime) {
	if (	m_pimpl->m_requestedDataStartTime != pt::not_a_date_time
			&& m_pimpl->m_requestedDataStartTime <= newTime) {
		return;
	}
	m_pimpl->m_requestedDataStartTime = newTime;
}

const pt::ptime & Security::GetRequestedDataStartTime() const {
	return m_pimpl->m_requestedDataStartTime;
}

double Security::GetLastPrice() const {
	return DescalePrice(GetLastPriceScaled());
}

ScaledPrice Security::GetLastPriceScaled() const {
	return GetIfSet<LEVEL1_TICK_LAST_PRICE>(m_pimpl->m_level1);
}

Qty Security::GetLastQty() const {
	return GetIfSet<LEVEL1_TICK_LAST_QTY>(m_pimpl->m_level1);
}

Qty Security::GetTradedVolume() const {
	return GetIfSet<LEVEL1_TICK_TRADING_VOLUME>(m_pimpl->m_level1);
}

ScaledPrice Security::GetAskPriceScaled() const {
	return GetIfSet<LEVEL1_TICK_ASK_PRICE>(m_pimpl->m_level1);
}

double Security::GetAskPrice() const {
	return DescalePrice(GetAskPriceScaled());
}

Qty Security::GetAskQty() const {
	return GetIfSet<LEVEL1_TICK_ASK_QTY>(m_pimpl->m_level1);
}

ScaledPrice Security::GetBidPriceScaled() const {
	return GetIfSet<LEVEL1_TICK_BID_PRICE>(m_pimpl->m_level1);
}

double Security::GetBidPrice() const {
	return DescalePrice(GetBidPriceScaled());
}

Qty Security::GetBidQty() const {
	return GetIfSet<LEVEL1_TICK_BID_QTY>(m_pimpl->m_level1);
}

Qty Security::GetBrokerPosition() const {
	return m_pimpl->m_brokerPosition;
}

Security::Level1UpdateSlotConnection Security::SubscribeToLevel1Updates(
			const Level1UpdateSlot &slot)
		const {
	return m_pimpl->m_level1UpdateSignal.connect(slot);
}

Security::Level1UpdateSlotConnection Security::SubscribeToLevel1Ticks(
			const Level1TickSlot &slot)
		const {
	return m_pimpl->m_level1TickSignal.connect(slot);
}

Security::NewTradeSlotConnection Security::SubscribeToTrades(
			const NewTradeSlot &slot)
		const {
	return m_pimpl->m_tradeSignal.connect(slot);
}

Security::NewTradeSlotConnection Security::SubscribeToBrokerPositionUpdates(
			const BrokerPositionUpdateSlot &slot)
		const {
	return m_pimpl->m_brokerPositionUpdateSignal.connect(slot);
}

Security::NewBarSlotConnection Security::SubscribeToBars(
			const NewBarSlot &slot)
		const {
	return m_pimpl->m_barSignal.connect(slot);
}

Security::BookUpdateTickSlotConnection
Security::SubscribeToBookUpdateTicks(const BookUpdateTickSlot &slot) const {
	return m_pimpl->m_bookUpdateTickSignal.connect(slot);
}

bool Security::IsLevel1Required() const {
	return IsLevel1UpdatesRequired() || IsLevel1TicksRequired();
}

bool Security::IsLevel1UpdatesRequired() const {
	return !m_pimpl->m_level1UpdateSignal.empty();
}

bool Security::IsLevel1TicksRequired() const {
	return !m_pimpl->m_level1TickSignal.empty();
}

bool Security::IsTradesRequired() const {
	return !m_pimpl->m_tradeSignal.empty();
}

bool Security::IsBrokerPositionRequired() const {
	return !m_pimpl->m_brokerPositionUpdateSignal.empty();
}

bool Security::IsBarsRequired() const {
	return !m_pimpl->m_barSignal.empty();
}

void Security::SetLevel1(
			const pt::ptime &time,
			const Level1TickValue &tick,
			const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->SetLevel1(time, tick, timeMeasurement, true, false);
}

void Security::SetLevel1(
			const pt::ptime &time,
			const Level1TickValue &tick1,
			const Level1TickValue &tick2,
			const TimeMeasurement::Milestones &timeMeasurement) {
	AssertNe(tick1.type, tick2.type);
	m_pimpl->SetLevel1(
		time,
		tick2,
		timeMeasurement,
		true,
		m_pimpl->SetLevel1(time, tick1, timeMeasurement, false, false));
}

void Security::SetLevel1(
			const pt::ptime &time,
			const Level1TickValue &tick1,
			const Level1TickValue &tick2,
			const Level1TickValue &tick3,
			const TimeMeasurement::Milestones &timeMeasurement) {
	AssertNe(tick1.type, tick2.type);
	AssertNe(tick1.type, tick3.type);
	AssertNe(tick2.type, tick3.type);
	m_pimpl->SetLevel1(
		time,
		tick3,
		timeMeasurement,
		true,
		m_pimpl->SetLevel1(
			time,
			tick2,
			timeMeasurement,
			false,
			m_pimpl->SetLevel1(time, tick1, timeMeasurement, false, false)));
}

void Security::SetLevel1(
			const pt::ptime &time,
			const Level1TickValue &tick1,
			const Level1TickValue &tick2,
			const Level1TickValue &tick3,
			const Level1TickValue &tick4,
			const TimeMeasurement::Milestones &timeMeasurement) {
	AssertNe(tick1.type, tick2.type);
	AssertNe(tick1.type, tick3.type);
	AssertNe(tick1.type, tick4.type);
	AssertNe(tick2.type, tick4.type);
	AssertNe(tick3.type, tick2.type);
	AssertNe(tick3.type, tick4.type);
	m_pimpl->SetLevel1(
		time,
		tick4,
		timeMeasurement,
		true,
		m_pimpl->SetLevel1(
			time,
			tick3,
			timeMeasurement,
			false,
			m_pimpl->SetLevel1(
				time,
				tick2,
				timeMeasurement,
				false,
				m_pimpl->SetLevel1(
					time,
					tick1,
					timeMeasurement,
					false,
					false))));
}

void Security::AddLevel1Tick(
			const pt::ptime &time,
			const Level1TickValue &tick,
			const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->AddLevel1Tick(time, tick, timeMeasurement, true, false);
}

void Security::AddLevel1Tick(
			const pt::ptime &time,
			const Level1TickValue &tick1,
			const Level1TickValue &tick2,
			const TimeMeasurement::Milestones &timeMeasurement) {
	AssertNe(tick1.type, tick2.type);
	m_pimpl->AddLevel1Tick(
		time,
		tick2,
		timeMeasurement,
		true,
		m_pimpl->AddLevel1Tick(time, tick1, timeMeasurement, false, false));
}

void Security::AddLevel1Tick(
			const pt::ptime &time,
			const Level1TickValue &tick1,
			const Level1TickValue &tick2,
			const Level1TickValue &tick3,
			const TimeMeasurement::Milestones &timeMeasurement) {
	AssertNe(tick1.type, tick2.type);
	AssertNe(tick1.type, tick3.type);
	AssertNe(tick2.type, tick3.type);
	m_pimpl->AddLevel1Tick(
		time,
		tick3,
		timeMeasurement,
		true,
		m_pimpl->AddLevel1Tick(
			time,
			tick2,
			timeMeasurement,
			false,
			m_pimpl->AddLevel1Tick(
				time,
				tick1,
				timeMeasurement,
				false,
				false)));
}

void Security::AddTrade(
			const boost::posix_time::ptime &time,
			OrderSide side,
			ScaledPrice price,
			Qty qty,
			const TimeMeasurement::Milestones &timeMeasurement,
			bool useAsLastTrade,
			bool useForTradedVolume) {
	
	bool isLevel1Changed = false;
	if (useAsLastTrade) {
		if (m_pimpl->SetLevel1(
				time,
				Level1TickValue::Create<LEVEL1_TICK_LAST_QTY>(qty),
				timeMeasurement,
				false,
				false)) {
			isLevel1Changed = true;
		}
		if (m_pimpl->SetLevel1(
				time,
				Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>(price),
				timeMeasurement,
				!useForTradedVolume,
				isLevel1Changed)) {
			isLevel1Changed = true;
		}
	}
	
	AssertLt(0, qty);
	if (useForTradedVolume && qty > 0) {
		for ( ; ; ) {
			const auto &prevVal = m_pimpl->m_level1[LEVEL1_TICK_TRADING_VOLUME];
			const auto newVal
				= Level1TickValue::Create<LEVEL1_TICK_TRADING_VOLUME>(
					IsSet(prevVal) ? Qty(prevVal) + qty : qty);
			if (	m_pimpl->CompareAndSetLevel1(
						time,
						newVal,
						prevVal,
						timeMeasurement,
						true,
						isLevel1Changed)) {
				break;
			}
		}
	}

	m_pimpl->m_tradeSignal(time, price, qty, side);

}

void Security::AddBar(const Bar &bar) {
	m_pimpl->m_barSignal(bar);
}

void Security::SetBrokerPosition(trdk::Qty qty, bool isInitial) {
	if (m_pimpl->m_brokerPosition.exchange(qty) == qty) {
		return;
	}
	m_pimpl->m_brokerPositionUpdateSignal(qty, isInitial);
}

bool Security::IsBookAdjusted() const {
	return m_pimpl->m_isBookAdjusted;
}

Security::BookUpdateOperation Security::StartBookUpdate(
		const pt::ptime &time,
		bool isRespected) {
	return BookUpdateOperation(*this, time, isRespected);
}

////////////////////////////////////////////////////////////////////////////////

class Security::Book::Side::Implementation : private boost::noncopyable {
public:
	std::vector<Security::Book::Level> m_levels;
};

Security::Book::Side::Side()
	: m_pimpl(new Implementation) {
	//...//
}

Security::Book::Side::~Side() {
	delete m_pimpl;
}

size_t Security::Book::Side::GetLevelsCount() const {
	return m_pimpl->m_levels.size();
}

const Security::Book::Level & Security::Book::Side::GetLevel(
		size_t index)
		const {
	if (index >= m_pimpl->m_levels.size()) {
		throw LogicError("Book price level is out of range");
	}
	return m_pimpl->m_levels[index];
}

///

Security::Book::Book(const pt::ptime &time, bool isAdjusted)
	: m_time(time),
	m_isAdjusted(isAdjusted) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

class Security::BookUpdateOperation::Implementation
		: private boost::noncopyable {

public:

	Security &m_security;

	const boost::shared_ptr<Security::Book> m_book;

	BookSideUpdateOperation m_bids;
	BookSideUpdateOperation m_offers;

	explicit Implementation(
			Security &security,
			const boost::shared_ptr<Security::Book> &book)
		: m_security(security),
		m_book(book),
		m_bids(m_book->m_bids),
		m_offers(m_book->m_offers) {
		//...//
	}

};

Security::BookSideUpdateOperation::BookSideUpdateOperation(
		Security::Book::Side &storage)
	: m_storage(storage) {
	//...//
}

void Security::BookSideUpdateOperation::Swap(
		std::vector<Security::Book::Level> &rhs) {
	rhs.swap(m_storage.m_pimpl->m_levels);
}

Security::BookUpdateOperation::BookUpdateOperation(
		Security &security,
		const pt::ptime &time,
		bool isAdjusted)
	: m_pimpl(
		new Implementation(
			security,
			boost::shared_ptr<Security::Book>(
				new Security::Book(time, isAdjusted)))) {
	//...//
}

Security::BookUpdateOperation::BookUpdateOperation(BookUpdateOperation &&rhs)
	: m_pimpl(rhs.m_pimpl) {
	rhs.m_pimpl = nullptr;
}

Security::BookUpdateOperation::~BookUpdateOperation() {
	delete m_pimpl;
}

void Security::BookUpdateOperation::Adjust() {
	if (m_pimpl->m_book->IsAdjusted()) {
		throw LogicError("Book already adjusted");
	}
	m_pimpl->m_book->m_isAdjusted = Adjust(
		m_pimpl->m_security,
		m_pimpl->m_bids.m_storage.m_pimpl->m_levels,
		m_pimpl->m_offers.m_storage.m_pimpl->m_levels);
}

bool Security::BookUpdateOperation::Adjust(
		const Security &security,
		std::vector<trdk::Security::Book::Level> &bids,
		std::vector<trdk::Security::Book::Level> &asks) {

	size_t count = 0;

	while (
			!bids.empty()
			&& !asks.empty()
			&& bids.front().GetPrice() > asks.front().GetPrice()) {
		
		bool isBidOlder = bids.front().GetTime() < asks.front().GetTime();
		
		security.GetSource().GetTradingLog().Write(
			"book\tadjust\t%1%\t%2% %3% %4%\t%5% %6% %7%\t%8%",
			[&](TradingRecord &record) {
				record
					% security
					% bids.front().GetPrice()
					% bids.front().GetTime()
					% (isBidOlder ? 'X' : '+')
					% asks.front().GetPrice()
					% asks.front().GetTime()
					% (isBidOlder ? '+' : 'X')
					% (count + 1);
			});
		
		if (isBidOlder) {
			bids.erase(bids.begin());
		} else {
			asks.erase(asks.begin());
		}
		
		++count;
	
	}

	return count > 0;

}

Security::BookSideUpdateOperation & Security::BookUpdateOperation::GetBids() {
	return m_pimpl->m_bids;
}

Security::BookSideUpdateOperation & Security::BookUpdateOperation::GetOffers() {
	return m_pimpl->m_offers;
}

Security::BookSideUpdateOperation & Security::BookUpdateOperation::GetAsks() {
	return m_pimpl->m_offers;
}

void Security::BookUpdateOperation::Commit(
		const TimeMeasurement::Milestones &timeMeasurement) {

#	if defined(BOOST_ENABLE_ASSERT_HANDLER)
	{
		for (
				size_t i = 0;
				i < m_pimpl->m_book->GetBids().GetLevelsCount();
				++i) {
			Assert(!IsZero(m_pimpl->m_book->GetBids().GetLevel(i).GetPrice()));
			Assert(!IsZero(m_pimpl->m_book->GetBids().GetLevel(i).GetQty()));
		}
		for (
				size_t i = 0;
				i < m_pimpl->m_book->GetAsks().GetLevelsCount();
				++i) {
			Assert(!IsZero(m_pimpl->m_book->GetAsks().GetLevel(i).GetPrice()));
			Assert(!IsZero(m_pimpl->m_book->GetAsks().GetLevel(i).GetQty()));
		}
	}
#	endif

	m_pimpl->m_security.SetLevel1(
		m_pimpl->m_book->GetTime(),
		Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(
			m_pimpl->m_book->GetBids().GetLevelsCount() == 0
				?	0
				:	m_pimpl->m_security.ScalePrice(m_pimpl->m_book->GetBids().GetLevel(0).GetPrice())),
		Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(
			m_pimpl->m_book->GetBids().GetLevelsCount() == 0
				?	0
				:	m_pimpl->m_book->GetBids().GetLevel(0).GetQty()),
		Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(
			m_pimpl->m_book->GetAsks().GetLevelsCount() == 0
				?	0
				:	m_pimpl->m_security.ScalePrice(m_pimpl->m_book->GetAsks().GetLevel(0).GetPrice())),
		Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(
			m_pimpl->m_book->GetAsks().GetLevelsCount() == 0
				?	0
				:	m_pimpl->m_book->GetAsks().GetLevel(0).GetQty()),
		timeMeasurement);

	m_pimpl->m_security.m_pimpl->m_isBookAdjusted
		= m_pimpl->m_book->IsAdjusted();
	m_pimpl->m_security.m_pimpl->m_book = m_pimpl->m_book;
	timeMeasurement.Measure(TimeMeasurement::SM_DISPATCHING_DATA_STORE);

	m_pimpl->m_security.m_pimpl->m_bookUpdateTickSignal(
		m_pimpl->m_book,
		timeMeasurement);

}

////////////////////////////////////////////////////////////////////////////////
