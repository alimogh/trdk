/**************************************************************************
 *   Created: 2012/10/27 15:05:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "MarketDataSource.hpp"

using namespace Trader::Lib;
using namespace Trader::Interaction::Csv;

MarketDataSource::MarketDataSource(
			const Trader::Lib::IniFile &ini,
			const std::string &section)
		: m_isStopped(true) {
	const std::string filePath = ini.ReadKey(section, "source", false);
	Log::Info(
		TRADER_INTERACTION_CSV_LOG_PREFFIX "loading file \"%1%\"...",
		filePath);
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
	m_thread.reset(new boost::thread([this]() {this->ReadFile();}));
}

void MarketDataSource::ReadFile() {

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
