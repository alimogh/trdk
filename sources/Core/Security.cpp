/**************************************************************************
 *   Created: 2012/07/09 18:42:14
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Security.hpp"
#include "Position.hpp"
#include "Settings.hpp"

namespace fs = boost::filesystem;
namespace lt = boost::local_time;
namespace pt = boost::posix_time;

using namespace Trader;
using namespace Trader::Lib;

//////////////////////////////////////////////////////////////////////////

class Security::Implementation : private boost::noncopyable {

public:

	typedef boost::shared_mutex MarketDataTimeMutex;
	typedef boost::shared_lock<MarketDataTimeMutex> MarketDataTimeReadLock;
	typedef boost::unique_lock<MarketDataTimeMutex> MarketDataTimeWriteLock;

public:

	class MarketDataLog : private boost::noncopyable {
	public:
		explicit MarketDataLog(const std::string &fullSymbol);
	public:
		void Append(
				const pt::ptime &timeOfReception,
				const pt::ptime &lastTradeTime,
				double last,
				double ask,
				double bid);
	private:
		std::ofstream m_file;
	};
	MarketDataLog *m_marketDataLog;

	mutable boost::signals2::signal<Level1UpdateSlotSignature> m_level1UpdateSignal;
	mutable boost::signals2::signal<NewTradeSlotSignature> m_tradeSignal;

	volatile long long m_lastPrice;
	volatile long m_lastSize;

	volatile long long m_askPrice;
	volatile long long m_askPrice2;
	volatile long m_askQty;
	volatile long m_askQty2;

	volatile long long m_bidPrice;
	volatile long long m_bidPrice2;
	volatile long m_bidQty;
	volatile long m_bidQty2;

	volatile long m_tradedVolume;

	mutable MarketDataTimeMutex m_marketDataTimeMutex;
	pt::ptime m_marketDataTime;

public:

	Implementation(
					const Instrument &instrument,
					bool logMarketData)
			: m_marketDataLog(nullptr),
			m_lastPrice(0),
			m_lastSize(0),
			m_askPrice(0),
			m_askPrice2(0),
			m_askQty(0),
			m_askQty2(0),
			m_bidPrice(0),
			m_bidPrice2(0),
			m_bidQty(0),
			m_bidQty2(0),
			m_tradedVolume(0) {
		if (logMarketData) {
			m_marketDataLog = new MarketDataLog(instrument.GetFullSymbol());
		}
	}

	~Implementation() {
		delete m_marketDataLog;
	}

};

//////////////////////////////////////////////////////////////////////////

Security::Implementation::MarketDataLog::MarketDataLog(
			const std::string &fullSymbol) {
	const fs::path filePath = Util::SymbolToFilePath(
		Defaults::GetMarketDataLogDir(),
		fullSymbol,
		"csv");
	const bool isNew = !fs::exists(filePath);
	if (isNew) {
		fs::create_directories(filePath.branch_path());
	}
	m_file.open(
		filePath.c_str(),
		std::ios::out | std::ios::ate | std::ios::app);
	if (!m_file) {
		Log::Error(
			"Failed to open log file %1% for security market data.",
			filePath);
		throw Exception("Failed to open log file for security market data");
	}
	Log::Info("Logging \"%1%\" market data into %2%...", fullSymbol, filePath);
	if (isNew) {
		m_file
			<< "time of reception,last trade time,last price,ask,bid"
			<< std::endl;
	}
}

void Security::Implementation::MarketDataLog::Append(
			const pt::ptime &timeOfReception,
			const pt::ptime &lastTradeTime,
			double last,
			double ask,
			double bid) {
	m_file
		<< (timeOfReception + Util::GetEdtDiff()).time_of_day()
		<< ',' << (lastTradeTime + Util::GetEdtDiff()).time_of_day()
		<< ',' << last
		<< ',' << ask
		<< ',' << bid
		<< std::endl;
}

//////////////////////////////////////////////////////////////////////////


Security::Security(
			boost::shared_ptr<TradeSystem> tradeSystem,
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			boost::shared_ptr<const Settings> settings,
			bool logMarketData)
		: Base(tradeSystem, symbol, primaryExchange, exchange, settings) {
	m_pimpl = new Implementation(*this, logMarketData);
}

Security::Security(
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			boost::shared_ptr<const Settings> settings,
			bool logMarketData)
		: Base(symbol, primaryExchange, exchange, settings) {
	m_pimpl = new Implementation(*this, logMarketData);
}

Security::~Security() {
	delete m_pimpl;
}

boost::posix_time::ptime Security::GetLastMarketDataTime() const {
	const Implementation::MarketDataTimeReadLock lock(
		m_pimpl->m_marketDataTimeMutex);
	Assert(!m_pimpl->m_marketDataTime.is_not_a_date_time());
	return m_pimpl->m_marketDataTime;
}

OrderId Security::SellAtMarketPrice(Qty qty, Position &position) {
	return GetTradeSystem().SellAtMarketPrice(
		*this,
		qty,
		position.GetSellOrderStatusUpdateSlot());
}

OrderId Security::Sell(
			Qty qty,
			ScaledPrice price,
			Position &position) {
	return GetTradeSystem().Sell(
		*this,
		qty,
		price,
		position.GetSellOrderStatusUpdateSlot());
}

OrderId Security::SellAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice,
			Position &position) {
	return GetTradeSystem().SellAtMarketPriceWithStopPrice(
		*this,
		qty,
		stopPrice,
		position.GetSellOrderStatusUpdateSlot());
}

OrderId Security::SellOrCancel(
			Qty qty,
			ScaledPrice price,
			Position &position) {
	return GetTradeSystem().SellOrCancel(
		*this,
		qty,
		price,
		position.GetSellOrderStatusUpdateSlot());
}

OrderId Security::BuyAtMarketPrice(Qty qty, Position &position) {
	return GetTradeSystem().BuyAtMarketPrice(
		*this,
		qty,
		position.GetBuyOrderStatusUpdateSlot());
}

OrderId Security::Buy(
			Qty qty,
			ScaledPrice price,
			Position &position) {
	return GetTradeSystem().Buy(
		*this,
		qty,
		price,
		position.GetBuyOrderStatusUpdateSlot());
}

OrderId Security::BuyAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice,
			Position &position) {
	return GetTradeSystem().BuyAtMarketPriceWithStopPrice(
		*this,
		qty,
		stopPrice,
		position.GetBuyOrderStatusUpdateSlot());
}

OrderId Security::BuyOrCancel(
			Qty qty,
			ScaledPrice price,
			Position &position) {
	return GetTradeSystem().BuyOrCancel(
		*this,
		qty,
		price,
		position.GetBuyOrderStatusUpdateSlot());
}

void Security::CancelOrder(OrderId orderId) {
	GetTradeSystem().CancelOrder(orderId);
}

void Security::CancelAllOrders() {
	GetTradeSystem().CancelAllOrders(*this);
}

Security::operator bool() const {
	return m_pimpl->m_lastPrice && m_pimpl->m_askPrice && m_pimpl->m_bidPrice;
}

ScaledPrice Security::GetLastPriceScaled() const {
	return m_pimpl->m_lastPrice;
}

void Security::SetLastMarketDataTime(const boost::posix_time::ptime &time) {
	const Implementation::MarketDataTimeWriteLock lock(
		m_pimpl->m_marketDataTimeMutex);
	if (	m_pimpl->m_marketDataTime >= time
			&& !m_pimpl->m_marketDataTime.is_not_a_date_time()) {
		return;
	}
	m_pimpl->m_marketDataTime = time;
}

bool Security::SetLast(double price, Qty size) {
	return SetLast(ScalePrice(price), size);
}

bool Security::SetAsk(double price, Qty size, size_t pos) {
	return SetAsk(ScalePrice(price), size, pos);
}

bool Security::SetBid(double price, Qty size, size_t pos) {
	return SetBid(ScalePrice(price), size, pos);
}

bool Security::SetLast(ScaledPrice price, Qty size) {
	bool isChanged = Interlocking::Exchange(m_pimpl->m_lastPrice, price) != price;
	isChanged = Interlocking::Exchange(
				m_pimpl->m_lastSize,
				size)
			!= size
		|| isChanged;
	return isChanged;
}

bool Security::SetAsk(ScaledPrice price, Qty qty, size_t pos) {
	volatile long long &pricePlace
		= pos == 1 ? m_pimpl->m_askPrice : m_pimpl->m_askPrice2;
	volatile long &qtyPlace = pos == 1 ? m_pimpl->m_askQty : m_pimpl->m_askQty2;
	bool isChanged = Interlocking::Exchange(pricePlace, price) != price;
	isChanged = Interlocking::Exchange(qtyPlace, qty) != qty || isChanged;
	return isChanged;
}

bool Security::SetBid(ScaledPrice price, Qty qty, size_t pos) {
	volatile long long &pricePlace
		= pos == 1 ? m_pimpl->m_bidPrice : m_pimpl->m_bidPrice2;
	volatile long &qtyPlace = pos == 1 ? m_pimpl->m_bidQty : m_pimpl->m_bidQty2;
	bool isChanged = Interlocking::Exchange(pricePlace, price) != price;
	isChanged = Interlocking::Exchange(qtyPlace, qty) != qty || isChanged;
	return isChanged;
}

double Security::GetLastPrice() const {
	return DescalePrice(GetLastPriceScaled());
}

Qty Security::GetLastQty() const {
	return m_pimpl->m_lastSize;
}

Qty Security::GetTradedVolume() const {
	return m_pimpl->m_tradedVolume;
}

ScaledPrice Security::GetAskPriceScaled(size_t pos) const {
	return pos == 1 ? m_pimpl->m_askPrice : m_pimpl->m_askPrice2;
}

double Security::GetAskPrice(size_t pos) const {
	return DescalePrice(GetAskPriceScaled(pos));
}

Qty Security::GetAskQty(size_t pos) const {
	return pos == 1 ? m_pimpl->m_askQty : m_pimpl->m_askQty2;
}

ScaledPrice Security::GetBidPriceScaled(size_t pos) const {
	return pos == 1 ? m_pimpl->m_bidPrice : m_pimpl->m_bidPrice2;
}

double Security::GetBidPrice(size_t pos) const {
	return DescalePrice(GetBidPriceScaled(pos));
}

Qty Security::GetBidQty(size_t pos) const {
	return pos == 1 ? m_pimpl->m_bidQty : m_pimpl->m_bidQty2;
}

Security::Level1UpdateSlotConnection Security::SubcribeToLevel1(
			const Level1UpdateSlot &slot)
		const {
	return Level1UpdateSlotConnection(
		m_pimpl->m_level1UpdateSignal.connect(slot));
}

Security::NewTradeSlotConnection Security::SubcribeToTrades(
			const NewTradeSlot &slot)
		const {
	return NewTradeSlotConnection(m_pimpl->m_tradeSignal.connect(slot));
}

void Security::SignalLevel1Update() {
	if (m_pimpl->m_marketDataLog) {
		m_pimpl->m_marketDataLog->Append(
			boost::get_system_time(),
			GetLastMarketDataTime(),
			DescalePrice(m_pimpl->m_lastPrice),
			DescalePrice(m_pimpl->m_askPrice),
			DescalePrice(m_pimpl->m_bidPrice));
	}
	m_pimpl->m_level1UpdateSignal();
}

void Security::SignalNewTrade(
			const boost::posix_time::ptime &time,
			OrderSide side,
			ScaledPrice price,
			Qty qty) {
	using namespace Interlocking;
	for ( ; ; ) {
		const auto prevVal = m_pimpl->m_tradedVolume;
		const auto newVal = prevVal + qty;
		if (CompareExchange(m_pimpl->m_tradedVolume, newVal, prevVal) == prevVal) {
			break;
		}
	}
	if (GetSettings().IsReplayMode()) {
		SetLastMarketDataTime(time);
	}
	m_pimpl->m_tradeSignal(time, price, qty, side);
}

//////////////////////////////////////////////////////////////////////////

std::ostream & std::operator <<(std::ostream &oss, const Security &security) {
	oss << security.GetFullSymbol();
	return oss;
}

//////////////////////////////////////////////////////////////////////////
