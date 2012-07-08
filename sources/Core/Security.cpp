/**************************************************************************
 *   Created: 2012/07/09 18:42:14
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Security.hpp"

namespace fs = boost::filesystem;
namespace lt = boost::local_time;

//////////////////////////////////////////////////////////////////////////

Security::Security(
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange)
		: Base(symbol, primaryExchange, exchange) {
	Interlocking::Exchange(m_last, 0);
	Interlocking::Exchange(m_ask, 0);
	Interlocking::Exchange(m_bid, 0);
}

Security::operator bool() const {
	return m_last && m_ask && m_bid;
}

Security::ScaledPrice Security::GetLastScaled() const {
	return m_last;
}

Security::ScaledPrice Security::GetAskScaled() const {
	return m_ask;
}

Security::ScaledPrice Security::GetBidScaled() const {
	return m_bid;
}

bool Security::SetLast(Price last) {
	return SetLast(Scale(last));
}

bool Security::SetAsk(Price ask) {
	return SetAsk(Scale(ask));
}

bool Security::SetBid(Price bid) {
	return SetBid(Scale(bid));
}

bool Security::SetLast(ScaledPrice last) {
	if (!last) {
		return false;
	}
	Interlocking::Exchange(m_last, last);
	return true;
}

bool Security::SetAsk(ScaledPrice ask) {
	if (!ask) {
		return false;
	}
	Interlocking::Exchange(m_ask, ask);
	return true;
}

bool Security::SetBid(ScaledPrice bid) {
	if (!bid) {
		return false;
	}
	Interlocking::Exchange(m_bid, bid);
	return true;
}

//////////////////////////////////////////////////////////////////////////

class DynamicSecurity::MarketDataLog : private boost::noncopyable {

private:
		
	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

public:

	explicit MarketDataLog(const std::string &fullSymbol) {
		const fs::path filePath = Util::SymbolToFilePath(
			Defaults::GetMarketDataLogDir(),
			fullSymbol,
			"log");
		const bool isNew = !fs::exists(filePath);
		if (isNew) {
			fs::create_directories(filePath.branch_path());
		}
		m_file.open(filePath.c_str());
		if (!m_file) {
			Log::Error(
				"Failed to open log file %1% for security market data.",
				filePath);
			throw Exception("Failed to open log file for security market data");
		}
		Log::Info("Logging \"%1%\" market data into %2%...", fullSymbol, filePath);
		if (isNew) {
			m_file << "data time,last price,ask,bid" << std::endl;
		} else {
			m_file << std::endl;
		}
	}

public:

	void Append(const MarketDataTime &time, Price last, Price ask, Price bid) {
		const lt::local_date_time esdTime(time, Util::GetEdtTimeZone());
		// const Lock lock(m_mutex); - not required while it uses only one IQLink thread
		m_file << esdTime << "," << last << "," << ask << "," << bid << std::endl;
	}

private:

	// mutable Mutex m_mutex;
	std::ofstream m_file;

};

DynamicSecurity::DynamicSecurity(
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange,
				bool logMarketData)
		: Base(symbol, primaryExchange, exchange) {
	if (logMarketData) {
		m_marketDataLog.reset(new MarketDataLog(GetFullSymbol()));
	}
	Interlocking::Exchange(m_isHistoryData, false);
}

void DynamicSecurity::Update(
				const MarketDataTime &time,
				Price last,
				Price ask,
				Price bid) {
	if (!SetLast(last) || !SetAsk(ask) || !SetBid(bid)) {
		return;
	}
	Assert(*this);
	if (!*this) {
		return;
	}
	m_updateSignal();
	if (m_marketDataLog) {
		m_marketDataLog->Append(time, last, ask, bid);
	}
}

DynamicSecurity::UpdateSlotConnection DynamicSecurity::Subcribe(
			const UpdateSlot &slot)
		const {
	return UpdateSlotConnection(m_updateSignal.connect(slot));
}

void DynamicSecurity::OnHistoryDataStart() {
	if (!Interlocking::Exchange(m_isHistoryData, true) && *this) {
		m_updateSignal();
	}
}

void DynamicSecurity::OnHistoryDataEnd() {
	if (Interlocking::Exchange(m_isHistoryData, false) && *this) {
		m_updateSignal();
	}
}
