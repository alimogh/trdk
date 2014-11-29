/**************************************************************************
 *   Created: 2012/10/27 15:05:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "CsvMarketDataSource.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Csv;

Csv::MarketDataSource::MarketDataSource(
			Context &context,
			const std::string &tag,
			const IniSectionRef &configuration)
		: Base(context, tag),
		m_pimaryExchange(configuration.ReadKey("exchange")),
		m_isStopped(true) {
	const auto filePath = configuration.ReadFileSystemPath("source");
	GetLog().Info(
		TRDK_INTERACTION_CSV_LOG_PREFFIX
			"loading file %1% for exchange \"%2%\"...",
		filePath,
		m_pimaryExchange);
	m_file.open(filePath.string().c_str());
	if (!m_file) {
		throw Exception("Failed to open CSV file");
	}
}

Csv::MarketDataSource::~MarketDataSource() {
	if (m_thread) {
		Assert(!m_isStopped);
		m_isStopped = true;
		try {
			m_thread->join();
		} catch (...) {
			AssertFailNoException();
		}
	}
}

void Csv::MarketDataSource::Connect(const IniSectionRef &) {
	Assert(m_isStopped);
	m_isStopped = false;
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

bool Csv::MarketDataSource::ParseTradeLine(
			const std::string &line,
			pt::ptime &time,
			OrderSide &side,
			std::string &symbol,
			std::string &exchange,
			ScaledPrice &price,
			Qty &qty)
		const {

	size_t field = 0;
	side = numberOfOrderSides;

	auto i = boost::make_split_iterator(
		line,
		boost::first_finder(",", boost::is_equal()));

	for ( ; !i.eof(); ++i) {
		switch (++field) {
			default:
				GetLog().Error(
					TRDK_INTERACTION_CSV_LOG_PREFFIX
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
					GetLog().Error(
						TRDK_INTERACTION_CSV_LOG_PREFFIX
							"format mismatch: empty symbol field.");
					return false;
				}
				break;
			case 4:
				exchange = boost::copy_range<std::string>(*i);
				boost::trim(exchange);
				if (exchange.empty()) {
					GetLog().Error(
						TRDK_INTERACTION_CSV_LOG_PREFFIX
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
						price = boost::lexical_cast<ScaledPrice>(val);
					} catch (const boost::bad_lexical_cast &ex) {
						GetLog().Error(
							TRDK_INTERACTION_CSV_LOG_PREFFIX
								"format mismatch: wrong price field value:"
								" %1% (%2%).",
							val,
							ex.what());
						return false;
					}
					if (price == 0) {
						GetLog().Error(
							TRDK_INTERACTION_CSV_LOG_PREFFIX
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
						qty = boost::lexical_cast<Qty>(val);
					} catch (const boost::bad_lexical_cast &ex) {
						GetLog().Error(
							TRDK_INTERACTION_CSV_LOG_PREFFIX
								"format mismatch:"
									" wrong quantity field value: %1% (%2%).",
							val,
							ex.what());
					}
					if (qty == 0) {
						GetLog().Error(
							TRDK_INTERACTION_CSV_LOG_PREFFIX
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
						GetLog().Error(
							TRDK_INTERACTION_CSV_LOG_PREFFIX
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
		GetLog().Error(
			TRDK_INTERACTION_CSV_LOG_PREFFIX
				"format mismatch: wrong field number (%1%)"
				" for primary exchange \"%2%\".",
			field,
			m_pimaryExchange);
		return false;
	}

	return true;

}

void Csv::MarketDataSource::ReadFile() {

	const auto &securityList = m_securityList.get<ByTradesRequirements>();
	if (securityList.find(true) == securityList.end()) {
		GetLog().Info(
			TRDK_INTERACTION_CSV_LOG_PREFFIX
				"reading stopped because it's not necessary.");
		return;
	}

	AssertLt(0, m_securityList.size());

	const auto &index = m_securityList.get<ByInstrument>();

	std::string line;
	size_t lineCount = 0;
	while (!m_isStopped && getline(m_file, line)) {

		const auto &timeMeasurement = GetContext().StartStrategyTimeMeasurement();

		++lineCount;

		pt::ptime time;
		OrderSide side = numberOfOrderSides;
		std::string symbol;
		std::string exchange;
		ScaledPrice price = 0;
		Qty qty = 0;
		if (!ParseTradeLine(line, time, side, symbol, exchange, price, qty)) {
			break;
		}

		//! @todo Place for optimization: 3 string objects copies every time
		const Symbol instrumnet(
			Symbol::SECURITY_TYPE_STOCK,
			symbol,
			exchange,
			m_pimaryExchange);
		const SecurityByInstrument::const_iterator security
			= index.find(instrumnet);
		if (security == index.end()) {
			GetLog().Debug(
				"Found unknown instrument: %1%:%2%:%3%.",
				symbol,
				m_pimaryExchange,
				exchange);
		} else if (security->security->IsTradesRequired()) {
			AssertNe(int(numberOfOrderSides), int(side));
			security
				->security
				->AddTrade(time, side, price, qty, timeMeasurement);
		}

    }

	GetLog().Info(
		TRDK_INTERACTION_CSV_LOG_PREFFIX
			"reading for exchange \"%1%\" is completed (line count: %2%).",
		m_pimaryExchange,
		lineCount);

}

trdk::Security & Csv::MarketDataSource::CreateSecurity(const Symbol &symbol) {
	boost::shared_ptr<Csv::Security> result(
		new Security(GetContext(), symbol, *this));
	Subscribe(result);
	return *result;
}
		
void Csv::MarketDataSource::Subscribe(
			const boost::shared_ptr<Security> &security)
		const {
	Assert(
		m_securityList.get<ByInstrument>().find(security->GetSymbol())
			== m_securityList.get<ByInstrument>().end());
 	const_cast<MarketDataSource *>(this)
 		->m_securityList.insert(SecurityHolder(security));
}
