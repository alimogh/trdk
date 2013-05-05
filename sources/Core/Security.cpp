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
#include "Position.hpp"
#include "Settings.hpp"
#include "Context.hpp"

namespace fs = boost::filesystem;
namespace lt = boost::local_time;
namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::Interlocking;

////////////////////////////////////////////////////////////////////////////////

namespace {
	
	template<typename T>
	bool IsSet(const T &val) {
		return val != std::numeric_limits<T>::max();
	}

	template<typename T>
	typename std::remove_volatile<T>::type GetIfSet(const T &val) {
		return IsSet(val) ? val : 0;
	}

	template<typename T>
	void Unset(T &val) {
		val = std::numeric_limits<T>::max();
	}

}

//////////////////////////////////////////////////////////////////////////

class Security::Implementation : private boost::noncopyable {

public:

	typedef boost::shared_mutex MarketDataTimeMutex;
	typedef boost::shared_lock<MarketDataTimeMutex> MarketDataTimeReadLock;
	typedef boost::unique_lock<MarketDataTimeMutex> MarketDataTimeWriteLock;

public:

	class MarketDataLog : private boost::noncopyable {
	public:
		explicit MarketDataLog(
					const Context &context,
					const std::string &fullSymbol);
	public:
		void Append(
				const pt::ptime &timeOfReception,
				const pt::ptime &lastTradeTime,
				double last,
				double ask,
				double bid);
	private:
		const Context &m_context;
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
			: m_marketDataLog(nullptr) {
		Unset(m_lastPrice);
		Unset(m_lastQty);
		Unset(m_askPrice);
		Unset(m_askQty);
		Unset(m_bidPrice);
		Unset(m_bidQty);
		Unset(m_tradedVolume);
		if (logMarketData) {
			m_marketDataLog = new MarketDataLog(
				instrument.GetContext(),
				instrument.GetFullSymbol());
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

	bool IsLevel1Started() const {
		return
			IsSet(m_lastPrice) && IsSet(m_lastQty)
			&& IsSet(m_askPrice) && IsSet(m_askQty)
			&& IsSet(m_bidPrice) && IsSet(m_bidQty);
	}

	void SignalLevel1Update() {
		if (!IsLevel1Started()) {
			return;
		}
		if (m_marketDataLog) {
			m_marketDataLog->Append(
				boost::get_system_time(),
				GetLastMarketDataTime(),
				DescalePrice(GetIfSet(m_lastPrice)),
				DescalePrice(GetIfSet(m_askPrice)),
				DescalePrice(GetIfSet(m_bidPrice)));
		}
		m_level1UpdateSignal();
	}

	bool SetBidAsk(
				ScaledPrice bidPrice,
				Qty bidQty,
				ScaledPrice askPrice,
				Qty askQty) {
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
			const Context &context,
			const std::string &fullSymbol)
		: m_context(context) {
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
		m_context.GetLog().Error(
			"Failed to open log file %1% for security market data.",
			filePath);
		throw Exception("Failed to open log file for security market data");
	}
	m_context.GetLog().Info(
		"Logging \"%1%\" market data into %2%...",
		boost::make_tuple(boost::cref(fullSymbol), boost::cref(filePath)));
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
			Context &context,
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			bool logMarketData)
		: Base(context, symbol, primaryExchange, exchange) {
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
	return GetContext().GetTradeSystem().SellAtMarketPrice(
		*this,
		qty,
		position.GetSellOrderStatusUpdateSlot());
}

OrderId Security::Sell(
			Qty qty,
			ScaledPrice price,
			Position &position) {
	return GetContext().GetTradeSystem().Sell(
		*this,
		qty,
		price,
		position.GetSellOrderStatusUpdateSlot());
}

OrderId Security::SellAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice,
			Position &position) {
	return GetContext().GetTradeSystem().SellAtMarketPriceWithStopPrice(
		*this,
		qty,
		stopPrice,
		position.GetSellOrderStatusUpdateSlot());
}

OrderId Security::SellOrCancel(
			Qty qty,
			ScaledPrice price,
			Position &position) {
	return GetContext().GetTradeSystem().SellOrCancel(
		*this,
		qty,
		price,
		position.GetSellOrderStatusUpdateSlot());
}

OrderId Security::BuyAtMarketPrice(Qty qty, Position &position) {
	return GetContext().GetTradeSystem().BuyAtMarketPrice(
		*this,
		qty,
		position.GetBuyOrderStatusUpdateSlot());
}

OrderId Security::Buy(
			Qty qty,
			ScaledPrice price,
			Position &position) {
	return GetContext().GetTradeSystem().Buy(
		*this,
		qty,
		price,
		position.GetBuyOrderStatusUpdateSlot());
}

OrderId Security::BuyAtMarketPriceWithStopPrice(
			Qty qty,
			ScaledPrice stopPrice,
			Position &position) {
	return GetContext().GetTradeSystem().BuyAtMarketPriceWithStopPrice(
		*this,
		qty,
		stopPrice,
		position.GetBuyOrderStatusUpdateSlot());
}

OrderId Security::BuyOrCancel(
			Qty qty,
			ScaledPrice price,
			Position &position) {
	return GetContext().GetTradeSystem().BuyOrCancel(
		*this,
		qty,
		price,
		position.GetBuyOrderStatusUpdateSlot());
}

void Security::CancelOrder(OrderId orderId) {
	GetContext().GetTradeSystem().CancelOrder(orderId);
}

void Security::CancelAllOrders() {
	GetContext().GetTradeSystem().CancelAllOrders(*this);
}

bool Security::IsStarted() const {
	return m_pimpl->IsLevel1Started();
}

ScaledPrice Security::GetLastPriceScaled() const {
	return GetIfSet(m_pimpl->m_lastPrice);
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
	return GetIfSet(m_pimpl->m_lastQty);
}

Qty Security::GetTradedVolume() const {
	return GetIfSet(m_pimpl->m_tradedVolume);
}

ScaledPrice Security::GetAskPriceScaled() const {
	return GetIfSet(m_pimpl->m_askPrice);
}

double Security::GetAskPrice() const {
	return DescalePrice(GetAskPriceScaled());
}

Qty Security::GetAskQty() const {
	return GetIfSet(m_pimpl->m_askQty);
}

ScaledPrice Security::GetBidPriceScaled() const {
	return GetIfSet(m_pimpl->m_bidPrice);
}

double Security::GetBidPrice() const {
	return DescalePrice(GetBidPriceScaled());
}

Qty Security::GetBidQty() const {
	return GetIfSet(m_pimpl->m_bidQty);
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

bool Security::IsLevel1Required() const {
	return !m_pimpl->m_level1UpdateSignal.empty();
}

bool Security::SetLastPrice(double price) {
	Assert(IsLevel1Required());
	const ScaledPrice scaledPrice = ScalePrice(price);
	if (Exchange(m_pimpl->m_lastPrice, scaledPrice) == scaledPrice) {
		return false;
	}
	m_pimpl->SignalLevel1Update();
	return true;
}

bool Security::SetLastQty(Qty qty) {
	Assert(IsLevel1Required());
	if (Exchange(m_pimpl->m_lastQty, qty) == qty) {
		return false;
	}
	m_pimpl->SignalLevel1Update();
	return true;
}

bool Security::SetVolume(Qty qty) {
	Assert(qty == 0 || IsLevel1Required());
	AssertLe(GetIfSet(m_pimpl->m_tradedVolume), qty);
	if (Exchange(m_pimpl->m_tradedVolume, qty) == qty) {
		return false;
	}
	m_pimpl->SignalLevel1Update();
	return true;
}

bool Security::SetBidPrice(double price) {
	Assert(IsLevel1Required());
	const ScaledPrice scaledPrice = ScalePrice(price);
	if (Exchange(m_pimpl->m_bidPrice, scaledPrice) == scaledPrice) {
		return false;
	}
	m_pimpl->SignalLevel1Update();
	return true;
}

bool Security::SetBidQty(Qty qty) {
	Assert(IsLevel1Required());
	if (Exchange(m_pimpl->m_bidQty, qty) == qty) {
		return false;
	}
	m_pimpl->SignalLevel1Update();
	return true;
}

bool Security::SetAskPrice(double price) {
	Assert(IsLevel1Required());
	const ScaledPrice scaledPrice = ScalePrice(price);
	if (Exchange(m_pimpl->m_askPrice, scaledPrice) == scaledPrice) {
		return false;
	}
	m_pimpl->SignalLevel1Update();
	return true;
}

bool Security::SetAskQty(Qty qty) {
	Assert(IsLevel1Required());
	if (Exchange(m_pimpl->m_askQty, qty) == qty) {
		return false;
	}
	m_pimpl->SignalLevel1Update();
	return true;
}

bool Security::SetBidAsk(
			ScaledPrice bidPrice,
			Qty bidQty,
			ScaledPrice askPrice,
			Qty askQty) {
	Assert(IsLevel1Required());
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
	Assert(IsLevel1Required());
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

bool Security::IsTradesRequired() const {
	return !m_pimpl->m_tradeSignal.empty();
}

void Security::AddTrade(
			const boost::posix_time::ptime &time,
			OrderSide side,
			ScaledPrice price,
			Qty qty,
			bool useAsLastTrade,
			bool useForTradedVolume) {
	
	Assert(IsTradesRequired());
	
	if (useForTradedVolume) {
		for ( ; ; ) {
			const auto prevVal = m_pimpl->m_tradedVolume;
			const auto newVal = prevVal + qty;
			if (	CompareExchange(m_pimpl->m_tradedVolume, newVal, prevVal)
					== prevVal) {
				break;
			}
		}
	}
	
	if (GetContext().GetSettings().IsReplayMode()) {
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
