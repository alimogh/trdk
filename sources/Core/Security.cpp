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
				const MarketDataTime &timeOfReception,
				const MarketDataTime &lastTradeTime,
				double last,
				double ask,
				double bid);
	private:
		std::ofstream m_file;
	};
	MarketDataLog *m_marketDataLog;

	mutable boost::signals2::signal<UpdateSlotSignature> m_updateSignal;
	mutable boost::signals2::signal<NewOrderSlotSignature> m_newOrderSignal;

	volatile long long m_isHistoryData;

	volatile long long m_lastPrice;
	volatile long m_lastSize;

	volatile long long m_askPrice;
	volatile long m_askQty;

	volatile long long m_bidPrice;
	volatile long m_bidQty;

	mutable MarketDataTimeMutex m_marketDataTimeMutex;
	MarketDataTime m_marketDataTime;

public:

	Implementation(
					const Instrument &instrument,
					bool logMarketData)
			: m_marketDataLog(nullptr),
			m_isHistoryData(false),
			m_lastPrice(0),
			m_lastSize(0),
			m_askPrice(0),
			m_askQty(0),
			m_bidPrice(0),
			m_bidQty(0) {
		if (logMarketData) {
			m_marketDataLog = new MarketDataLog(instrument.GetFullSymbol());
		}
	}

	~Implementation() {
		delete m_marketDataLog;
	}

};

//////////////////////////////////////////////////////////////////////////

Security::Implementation::MarketDataLog::MarketDataLog(const std::string &fullSymbol) {
	const fs::path filePath = Util::SymbolToFilePath(
		Defaults::GetMarketDataLogDir(),
		fullSymbol,
		"csv");
	const bool isNew = !fs::exists(filePath);
	if (isNew) {
		fs::create_directories(filePath.branch_path());
	}
	m_file.open(filePath.c_str(), std::ios::out | std::ios::ate | std::ios::app);
	if (!m_file) {
		Log::Error(
			"Failed to open log file %1% for security market data.",
			filePath);
		throw Exception("Failed to open log file for security market data");
	}
	Log::Info("Logging \"%1%\" market data into %2%...", fullSymbol, filePath);
	if (isNew) {
		m_file << "time of reception,last trade time,last price,ask,bid" << std::endl;
	}
}

void Security::Implementation::MarketDataLog::Append(
			const MarketDataTime &timeOfReception,
			const MarketDataTime &lastTradeTime,
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
	const Implementation::MarketDataTimeReadLock lock(m_pimpl->m_marketDataTimeMutex);
	Assert(!m_pimpl->m_marketDataTime.is_not_a_date_time());
	return m_pimpl->m_marketDataTime;
}

Security::OrderId Security::SellAtMarketPrice(Qty qty, Position &position) {
	return GetTradeSystem().SellAtMarketPrice(
		*this,
		qty,
		position.GetSellOrderStatusUpdateSlot());
}

Security::OrderId Security::Sell(Qty qty, ScaledPrice price, Position &position) {
	return GetTradeSystem().Sell(
		*this,
		qty,
		price,
		position.GetSellOrderStatusUpdateSlot());
}

Security::OrderId Security::SellAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice,
			Position &position) {
	return GetTradeSystem().SellAtMarketPriceWithStopPrice(
		*this,
		qty,
		stopPrice,
		position.GetSellOrderStatusUpdateSlot());
}

Security::OrderId Security::SellOrCancel(
			Qty qty,
			ScaledPrice price,
			Position &position) {
	return GetTradeSystem().SellOrCancel(
		*this,
		qty,
		price,
		position.GetSellOrderStatusUpdateSlot());
}

Security::OrderId Security::BuyAtMarketPrice(Qty qty, Position &position) {
	return GetTradeSystem().BuyAtMarketPrice(
		*this,
		qty,
		position.GetBuyOrderStatusUpdateSlot());
}

Security::OrderId Security::Buy(Qty qty, ScaledPrice price, Position &position) {
	return GetTradeSystem().Buy(
		*this,
		qty,
		price,
		position.GetBuyOrderStatusUpdateSlot());
}

Security::OrderId Security::BuyAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice,
			Position &position) {
	return GetTradeSystem().BuyAtMarketPriceWithStopPrice(
		*this,
		qty,
		stopPrice,
		position.GetBuyOrderStatusUpdateSlot());
}

Security::OrderId Security::BuyOrCancel(
			Qty qty,
			ScaledPrice price,
			Position &position) {
	return GetTradeSystem().BuyOrCancel(
		*this,
		qty,
		price,
		position.GetBuyOrderStatusUpdateSlot());
}

void Security::CancelOrder(Security::OrderId orderId) {
	GetTradeSystem().CancelOrder(orderId);
}

void Security::CancelAllOrders() {
	GetTradeSystem().CancelAllOrders(*this);
}

Security::operator bool() const {
	return m_pimpl->m_lastPrice && m_pimpl->m_askPrice && m_pimpl->m_bidPrice;
}

Security::ScaledPrice Security::GetLastPriceScaled() const {
	return m_pimpl->m_lastPrice;
}

void Security::SetLastMarketDataTime(const boost::posix_time::ptime &time) {
	const Implementation::MarketDataTimeWriteLock lock(m_pimpl->m_marketDataTimeMutex);
	m_pimpl->m_marketDataTime = time;
}

bool Security::SetLast(double price, Qty size) {
	return SetLast(ScalePrice(price), size);
}

bool Security::SetAsk(double price, Qty size) {
	return SetAsk(ScalePrice(price), size);
}

bool Security::SetBid(double price, Qty size) {
	return SetBid(ScalePrice(price), size);
}

bool Security::SetLast(ScaledPrice price, Qty size) {
	bool isChanged = Interlocking::Exchange(m_pimpl->m_lastPrice, price) != price;
	isChanged = Interlocking::Exchange(m_pimpl->m_lastSize, size) != size || isChanged;
	return isChanged;
}

bool Security::SetAsk(ScaledPrice price, Qty size) {
	Assert((!price && !size) || (price && size));
	if (!price || !size) {
		return false;
	}
	bool isChanged = Interlocking::Exchange(m_pimpl->m_askPrice, price) != price;
	isChanged = Interlocking::Exchange(m_pimpl->m_askQty, size) != size || isChanged;
	return isChanged;
}

bool Security::SetBid(ScaledPrice price, Qty size) {
	Assert((!price && !size) || (price && size));
	if (!price || !size) {
		return false;
	}
	bool isChanged = Interlocking::Exchange(m_pimpl->m_bidPrice, price) != price;
	isChanged = Interlocking::Exchange(m_pimpl->m_askQty, size) != size || isChanged;
	return isChanged;
}

double Security::GetLastPrice() const {
	return DescalePrice(GetLastPriceScaled());
}

Security::Qty Security::GetLastQty() const {
	return m_pimpl->m_lastSize;
}

Security::Qty Security::GetTradedVolume() const {
	return 0;
}

Security::ScaledPrice Security::GetAskPriceScaled() const {
	return m_pimpl->m_askPrice;
}

double Security::GetAskPrice() const {
	return DescalePrice(GetAskPriceScaled());
}

Security::Qty Security::GetAskQty() const {
	return m_pimpl->m_askQty;
}

Security::ScaledPrice Security::GetBidPriceScaled() const {
	return m_pimpl->m_bidPrice;
}

double Security::GetBidPrice() const {
	return DescalePrice(GetBidPriceScaled());
}

Security::Qty Security::GetBidQty() const {
	return m_pimpl->m_bidQty;
}

bool Security::IsHistoryData() const {
	return m_pimpl->m_isHistoryData
		?	!GetSettings().IsReplayMode()
		:	false;
}

void Security::OnHistoryDataStart() {
	if (!Interlocking::Exchange(m_pimpl->m_isHistoryData, true) && *this) {
		m_pimpl->m_updateSignal();
	}
}

void Security::OnHistoryDataEnd() {
	if (Interlocking::Exchange(m_pimpl->m_isHistoryData, false) && *this) {
		m_pimpl->m_updateSignal();
	}
}

Security::UpdateSlotConnection Security::Subcribe(const UpdateSlot &slot) const {
	return UpdateSlotConnection(m_pimpl->m_updateSignal.connect(slot));
}

Security::NewOrderSlotConnection Security::Subcribe(const NewOrderSlot &slot) const {
	return NewOrderSlotConnection(m_pimpl->m_newOrderSignal.connect(slot));
}

void Security::SignalUpdate() {
	if (m_pimpl->m_marketDataLog) {
		m_pimpl->m_marketDataLog->Append(
			boost::get_system_time(),
			GetLastMarketDataTime(),
			DescalePrice(m_pimpl->m_lastPrice),
			DescalePrice(m_pimpl->m_askPrice),
			DescalePrice(m_pimpl->m_bidPrice));
	}
	m_pimpl->m_updateSignal();
}

void Security::SignalNewOrder(
			const boost::posix_time::ptime &time,
			bool isBuy,
			ScaledPrice price,
			Qty qty) {
	m_pimpl->m_newOrderSignal(time, price, qty, isBuy);
}
