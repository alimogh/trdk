/**************************************************************************
 *   Created: 2012/10/27 15:05:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "CsvMarketDataSource.hpp"

namespace pt = boost::posix_time;
using namespace Trader::Lib;
using namespace Trader::Interaction::Csv;

MarketDataSource::MarketDataSource(
			const Trader::Lib::IniFile &ini,
			const std::string &section)
		: m_pimaryExchange(ini.ReadKey(section, "exchange", false)),
		m_isStopped(true) {
	const std::string filePath = ini.ReadKey(section, "source", false);
	Log::Info(
		TRADER_INTERACTION_CSV_LOG_PREFFIX
			"loading file \"%1%\" for exchange \"%2%\"...",
		filePath,
		m_pimaryExchange);
	m_file.open(filePath.c_str());
	if (!m_file) {
		throw Exception("Failed to open CSV file");
	}
}

MarketDataSource::~MarketDataSource() {
	if (m_thread) {
		Verify(!Interlocking::Exchange(m_isStopped, true));
		try {
			m_thread->join();
		} catch (...) {
			AssertFailNoException();
		}
	}
}

void MarketDataSource::Connect() {
	Verify(Interlocking::Exchange(m_isStopped, false));
	m_thread.reset(
		new boost::thread([this]() {
			try {
				this->ReadFile();
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		}));
}

bool MarketDataSource::ParseTradeLine(
			const std::string &line,
			pt::ptime &time,
			Trader::OrderSide &side,
			std::string &symbol,
			std::string &exchange,
			Trader::ScaledPrice &price,
			Trader::Qty &qty)
		const {

	size_t field = 0;
	side = numberOfOrderSides;

	auto i = boost::make_split_iterator(
		line,
		boost::first_finder(",", boost::is_equal()));

	for ( ; !i.eof(); ++i) {
		switch (++field) {
			default:
				Log::Error(
					TRADER_INTERACTION_CSV_LOG_PREFFIX
						"format mismatch: unknown field #%1%.",
					field);
				return false;
			case 1:
			case 7:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
				break;
			case 2:
				{
					std::string timeStr = boost::copy_range<std::string>(*i);
					boost::trim(timeStr);
					//! @todo Local time, not UTC
					time = pt::second_clock::local_time();
					time -= time.time_of_day();
					time += pt::duration_from_string(timeStr);
				}
				break;
			case 3:
				symbol = boost::copy_range<std::string>(*i);
				boost::trim(symbol);
				if (symbol.empty()) {
					Log::Error(
						TRADER_INTERACTION_CSV_LOG_PREFFIX
							"format mismatch: empty symbol field.");
					return false;
				}
				break;
			case 4:
				exchange = boost::copy_range<std::string>(*i);
				boost::trim(exchange);
				if (exchange.empty()) {
					Log::Error(
						TRADER_INTERACTION_CSV_LOG_PREFFIX
							"format mismatch: empty exchange field.");
					return false;
				}
				break;
			case 5:
				AssertEq(symbol, boost::copy_range<std::string>(*i));
				break;
			case 6:
				{
					auto val = boost::copy_range<std::string>(*i);
					boost::trim(val);
					try {
						price = boost::lexical_cast<Trader::ScaledPrice>(val);
					} catch (const boost::bad_lexical_cast &ex) {
						Log::Error(
							TRADER_INTERACTION_CSV_LOG_PREFFIX
								"format mismatch: wrong price field value:"
								" %1% (%2%).",
							val,
							ex.what());
						return false;
					}
					if (price == 0) {
						Log::Error(
							TRADER_INTERACTION_CSV_LOG_PREFFIX
								"format mismatch: wrong price field value: %1%.",
							val);
						return false;
					}
				}
				break;
			case 8:
				{
					auto val = boost::copy_range<std::string>(*i);
					boost::trim(val);
					try {
						qty = boost::lexical_cast<Trader::Qty>(val);
					} catch (const boost::bad_lexical_cast &ex) {
						Log::Error(
							TRADER_INTERACTION_CSV_LOG_PREFFIX
								"format mismatch:"
									" wrong quantity field value: %1% (%2%).",
							val,
							ex.what());
					}
					if (qty == 0) {
						Log::Error(
							TRADER_INTERACTION_CSV_LOG_PREFFIX
								"format mismatch:"
								" wrong quantity field value: %1%.",
							val);
						return false;
					}
				}
				break;
			case 16:
				{
					auto val = boost::copy_range<std::string>(*i);
					boost::trim(val);
					Assert(val == "S" || val == "B");
					if (val == "S") {
						side = ORDER_SIDE_SELL;
					} else if (val == "B") {
						side = ORDER_SIDE_BUY;
					} else {
						Log::Error(
							TRADER_INTERACTION_CSV_LOG_PREFFIX
								"format mismatch:"
								" unknown side field value: \"%1%\".",
							val);
						return false;
					}
				}
				break;
		}
	}

	if (field != 16) {
		Log::Error(
			TRADER_INTERACTION_CSV_LOG_PREFFIX
				"format mismatch: wrong field number (%1%) for primary exchange \"%2%\".",
			field,
			m_pimaryExchange);
		return false;
	}

	return true;

}

void MarketDataSource::ReadFile() {

	AssertLt(0, m_securityList.size());

	const SecurityByInstrument &index = m_securityList.get<ByInstrument>();

	std::string line;
	size_t lineCount = 0;
	while (getline(m_file, line)) {

		++lineCount;

		pt::ptime time;
		OrderSide side = numberOfOrderSides;
		std::string symbol;
		std::string exchange;
		Trader::ScaledPrice price = 0;
		Trader::Qty qty = 0;
		if (!ParseTradeLine(line, time, side, symbol, exchange, price, qty)) {
			break;
		}

		const SecurityByInstrument::const_iterator security
			= index.find(boost::make_tuple(symbol, m_pimaryExchange, exchange));
		if (security == index.end()) {
			Log::DebugEx(
				[&]() -> boost::format {
					boost::format message(
						"Found unknown instrument: %1%:%2%:%3%.");
					message % symbol % m_pimaryExchange % exchange;
					return message;
				});
		} else {
			AssertNe(int(Trader::numberOfOrderSides), int(side));
			security->security->AddTrade(time, side, price, qty);
		}

    }

	Log::Info(
		TRADER_INTERACTION_CSV_LOG_PREFFIX
			"reading for exchange \"%1%\" is completed (line count: %2%).",
		m_pimaryExchange,
		lineCount);

}

boost::shared_ptr<Trader::Security> MarketDataSource::CreateSecurity(
			boost::shared_ptr<Trader::TradeSystem> tradeSystem,
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			boost::shared_ptr<const Trader::Settings> settings,
			bool logMarketData)
		const {
	boost::shared_ptr<Security> result(
		new Security(
			tradeSystem,
			symbol,
			primaryExchange,
			exchange,
			settings,
			logMarketData));
	Subscribe(result);
	return result;
}
		
boost::shared_ptr<Trader::Security> MarketDataSource::CreateSecurity(
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			boost::shared_ptr<const Trader::Settings> settings,
			bool logMarketData)
		const {
	boost::shared_ptr<Security> result(
		new Security(
			symbol,
			primaryExchange,
			exchange,
			settings,
			logMarketData));	
	Subscribe(result);
	return result;
}

void MarketDataSource::Subscribe(
			boost::shared_ptr<Security> security)
		const {
	Assert(
		m_securityList.get<ByInstrument>().find(
				boost::make_tuple(
					security->GetSymbol(),
					security->GetPrimaryExchange(),
					security->GetExchange()))
			== m_securityList.get<ByInstrument>().end());
 	const_cast<MarketDataSource *>(this)
 		->m_securityList.insert(SecurityHolder(security));
}
