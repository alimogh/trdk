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

	mutable boost::signals2::signal<Level1UpdateSlotSignature>
		m_level1UpdateSignal;
	mutable boost::signals2::signal<NewTradeSlotSignature> m_tradeSignal;

	volatile long long m_lastPrice;
	volatile long m_lastQty;

	volatile long long m_askPrice;
	volatile long m_askQty;

	volatile long long m_bidPrice;
	volatile long m_bidQty;

	volatile long m_tradedVolume;

	mutable MarketDataTimeMutex m_marketDataTimeMutex;
	pt::ptime m_marketDataTime;

public:

	Implementation(
					const Instrument &instrument,
					bool logMarketData)
			: m_marketDataLog(nullptr),
			m_lastPrice(0),
			m_lastQty(0),
			m_askPrice(0),
			m_askQty(0),
			m_bidPrice(0),
			m_bidQty(0),
			m_tradedVolume(0) {
		if (logMarketData) {
			m_marketDataLog = new MarketDataLog(instrument.GetFullSymbol());
		}
	}

	~Implementation() {
		delete m_marketDataLog;
	}

	unsigned int GetPriceScale() const throw() {
		return 100;
	}

	ScaledPrice ScalePrice(double price) const {
		return Scale(price, GetPriceScale());
	}

	double DescalePrice(ScaledPrice price) const {
		return Descale(price, GetPriceScale());
	}

	pt::ptime GetLastMarketDataTime() const {
		const MarketDataTimeReadLock lock(m_marketDataTimeMutex);
		Assert(!m_marketDataTime.is_not_a_date_time());
		return m_marketDataTime;
	}

	void SignalLevel1Update() {
		if (m_marketDataLog) {
			m_marketDataLog->Append(
				boost::get_system_time(),
				GetLastMarketDataTime(),
				DescalePrice(m_lastPrice),
				DescalePrice(m_askPrice),
				DescalePrice(m_bidPrice));
		}
		m_level1UpdateSignal();
	}

	bool SetBidAsk(
				ScaledPrice bidPrice,
				Qty bidQty,
				ScaledPrice askPrice,
				Qty askQty) {
		using namespace Interlocking;
		bool isChanged = false;
		if (Exchange(m_bidPrice, bidPrice) != bidPrice) {
			isChanged = true;
		}
		if (Exchange(m_bidQty, bidQty) != bidQty) {
			isChanged = true;
		}
		if (Exchange(m_askPrice, askPrice) != askPrice) {
			isChanged = true;
		}
		if (Exchange(m_askQty, askQty) != askQty) {
			isChanged = true;
		}
		return isChanged;
	}

	bool SetLast(ScaledPrice price, Qty qty) {
		using namespace Interlocking;
		bool isChanged = false;
		if (Exchange(m_lastPrice, price) != price) {
			isChanged = true;
		}
		if (Exchange(m_lastQty, qty) != qty) {
			isChanged = true;
		}
		return isChanged;
	}

};

//////////////////////////////////////////////////////////////////////////

Security::Implementation::MarketDataLog::MarketDataLog(
			const std::string &fullSymbol) {
	const fs::path filePath = SymbolToFilePath(
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
		<< (timeOfReception + GetEdtDiff()).time_of_day()
		<< ',' << (lastTradeTime + GetEdtDiff()).time_of_day()
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

unsigned int Security::GetPriceScale() const throw() {
	return m_pimpl->GetPriceScale();
}

ScaledPrice Security::ScalePrice(double price) const {
	return m_pimpl->ScalePrice(price);
}

double Security::DescalePrice(ScaledPrice price) const {
	return m_pimpl->DescalePrice(price);
}

pt::ptime Security::GetLastMarketDataTime() const {
	return m_pimpl->GetLastMarketDataTime();
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

double Security::GetLastPrice() const {
	return DescalePrice(GetLastPriceScaled());
}

Qty Security::GetLastQty() const {
	return m_pimpl->m_lastQty;
}

Qty Security::GetTradedVolume() const {
	return m_pimpl->m_tradedVolume;
}

ScaledPrice Security::GetAskPriceScaled() const {
	return m_pimpl->m_askPrice;
}

double Security::GetAskPrice() const {
	return DescalePrice(GetAskPriceScaled());
}

Qty Security::GetAskQty() const {
	return m_pimpl->m_askQty;
}

ScaledPrice Security::GetBidPriceScaled() const {
	return m_pimpl->m_bidPrice;
}

double Security::GetBidPrice() const {
	return DescalePrice(GetBidPriceScaled());
}

Qty Security::GetBidQty() const {
	return m_pimpl->m_bidQty;
}

Security::Level1UpdateSlotConnection Security::SubcribeToLevel1(
			const Level1UpdateSlot &slot)
		const {
	return m_pimpl->m_level1UpdateSignal.connect(slot);
}

Security::NewTradeSlotConnection Security::SubcribeToTrades(
			const NewTradeSlot &slot)
		const {
	return m_pimpl->m_tradeSignal.connect(slot);
}

bool Security::SetBidAsk(
			ScaledPrice bidPrice,
			Qty bidQty,
			ScaledPrice askPrice,
			Qty askQty) {
	if (!m_pimpl->SetBidAsk(bidPrice, bidQty, askPrice, askQty)) {
		return false;
	}
	m_pimpl->SignalLevel1Update();
	return true;
}

bool Security::SetBidAsk(
			double bidPrice,
			Qty bidQty,
			double askPrice,
			Qty askQty) {
	return SetBidAsk(
		ScalePrice(bidPrice),
		bidQty,
		ScalePrice(askPrice),
		askQty);
}

bool Security::SetBidAskLast(
			ScaledPrice bidPrice,
			Qty bidQty,
			ScaledPrice askPrice,
			Qty askQty,
			ScaledPrice lastTradePrice,
			Qty lastTradeQty) {
	bool isChanged = false;
	if (m_pimpl->SetBidAsk(bidPrice, bidQty, askPrice, askQty)) {
		isChanged = true;
	}
	if (m_pimpl->SetLast(lastTradePrice, lastTradeQty)) {
		isChanged = true;
	}
	if (isChanged) {
		m_pimpl->SignalLevel1Update();
	}
	return isChanged;
}

bool Security::SetBidAskLast(
			double bidPrice,
			Qty bidQty,
			double askPrice,
			Qty askQty,
			double lastPrice,
			Qty lastQty) {
	return SetBidAskLast(
		ScalePrice(bidPrice),
		bidQty,
		ScalePrice(askPrice),
		askQty,
		ScalePrice(lastPrice),
		lastQty);
}

void Security::AddTrade(
			const boost::posix_time::ptime &time,
			OrderSide side,
			ScaledPrice price,
			Qty qty,
			bool useAsLastTrade) {

	for ( ; ; ) {
		const auto prevVal = m_pimpl->m_tradedVolume;
		const auto newVal = prevVal + qty;
		if (	Interlocking::CompareExchange(
						m_pimpl->m_tradedVolume,
						newVal,
						prevVal)
					== prevVal) {
			break;
		}
	}
	
	if (GetSettings().IsReplayMode()) {
		SetLastMarketDataTime(time);
	}

	if (useAsLastTrade && m_pimpl->SetLast(price, qty)) {
		m_pimpl->SignalLevel1Update();
	}

	m_pimpl->m_tradeSignal(time, price, qty, side);

}

//////////////////////////////////////////////////////////////////////////

std::ostream & std::operator <<(std::ostream &oss, const Security &security) {
	oss << security.GetFullSymbol();
	return oss;
}

//////////////////////////////////////////////////////////////////////////
