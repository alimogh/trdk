/**************************************************************************
 *   Created: 2012/09/11 18:54:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "MarketDataSource.hpp"
#include "Core/Security.hpp"
#include "Common/IniFile.hpp"
#include "FeedHandler.hpp"
#include "Security.hpp"

namespace fs = boost::filesystem;
using namespace Trader::Interaction::Enyx;

MarketDataSource::MarketDataSource() {
	//...//
}

MarketDataSource::~MarketDataSource() {
	//...///
}

void MarketDataSource::Connect(const IniFile &ini, const std::string &section) {

	Log::Info(TRADER_ENYX_LOG_PREFFIX "connecting...");

	Assert(!m_enyx);
	if (m_enyx) {
		Log::Warn(TRADER_ENYX_LOG_PREFFIX "already connected.");
		return;
	}

    if (!Dictionary::isFeedSupported("NASDAQ-ITCH")) {
        Log::Error(TRADER_ENYX_LOG_PREFFIX "NASDAQ-ITCH Feed is not supported.");
        throw ConnectError();
    }

    FeedSupport *totalViewNasdaqFeedSupport
		= Dictionary::getFeedByName("NASDAQ-ITCH", 4, 1);
    if (!totalViewNasdaqFeedSupport) {
        Log::Error(TRADER_ENYX_LOG_PREFFIX "can't retrieve FeedSupport for NASDAQ");
        throw ConnectError();
    }
    Log::Debug(
		TRADER_ENYX_LOG_PREFFIX "building Instrument List from \"%1%\".",
		totalViewNasdaqFeedSupport->getListingFilePath());

    totalViewNasdaqFeedSupport->updateInstruments();

    std::unique_ptr<EnyxMDInterface> enyx;
	if (ini.IsKeyExist(section, "pcap")) {
		const fs::path pcapFilePath = ini.ReadKey(section, "pcap", true);
		Log::Info(TRADER_ENYX_LOG_PREFFIX "loading PCAP-file %1%...", pcapFilePath);
        try {
			enyx.reset(
				new EnyxMDPcapInterface(
					pcapFilePath.string(),
					totalViewNasdaqFeedSupport,
					false));
        } catch (const BadPcapFileException &ex) {
			Log::Error(TRADER_ENYX_LOG_PREFFIX "failed to open PCAP-file: \"%1%\".", ex.what());
            throw ConnectError();
        }
	} else {
		Log::Info(TRADER_ENYX_LOG_PREFFIX "retrieve data from HARDWARE...");
        try {
            EnyxMDHwInterface &hardware
				= EnyxMD::getHardwareInterfaceForFeed("NASDAQ-ITCH");
            EnyxPort &port = hardware.getPortConfiguration();
            port.setInterfaceForLane(ENYXMD_LANE_C, "enyxnet0");
            enyx.reset(&hardware);
        } catch (const UnsuportedFeedException &ex) {
			Log::Error(
				TRADER_ENYX_LOG_PREFFIX "failed to make hardware connection: \"%1%\".",
				ex.what());
			throw ConnectError();
        }
	}

    enyx->setSecondStreamLane(ENYXMD_LANE_C);
    enyx->setInstrumentFiltering(true);
    enyx->setEmptyPacketDrop(true);

	std::unique_ptr<FeedHandler> handler(new FeedHandler);
    enyx->addHandler(*handler);

	m_enyx.reset(enyx.release());
	m_handler.reset(handler.release());
	Log::Info(TRADER_ENYX_LOG_PREFFIX "connected.");

}

void MarketDataSource::Start() {
	Log::Info(TRADER_ENYX_LOG_PREFFIX "starting...");
	Assert(m_enyx);
	m_enyx->start();
	Log::Info(TRADER_ENYX_LOG_PREFFIX "started.");
}

bool MarketDataSource::IsSupported(const Trader::Security &security) const {
	return Dictionary::isExchangeSupported(security.GetPrimaryExchange());
}

void MarketDataSource::CheckSupport(const Trader::Security &security) const {
	if (!IsSupported(security)) {
		boost::format message("Exchange \"%1%\" not supported by market data source");
		message % security.GetPrimaryExchange();
		throw Error(message.str().c_str());
	}
}

boost::shared_ptr<Trader::Security> MarketDataSource::CreateSecurity(
			boost::shared_ptr<Trader::TradeSystem> ts,
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			boost::shared_ptr<const Trader::Settings> settings,
			bool logMarketData)
		const {
	boost::shared_ptr<Security> result(
		new Security(ts, symbol, primaryExchange, exchange, settings, logMarketData));
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
		new Security(symbol, primaryExchange, exchange, settings, logMarketData));
	Subscribe(result);
	return result;
}

void MarketDataSource::Subscribe(boost::shared_ptr<Trader::Security> security) const {
	Log::Info(TRADER_ENYX_LOG_PREFFIX "subscribing \"%1%\"...", security->GetFullSymbol());
	CheckSupport(*security);
	const auto exchange = Dictionary::getExchangeByName(security->GetPrimaryExchange());
	if (!m_enyx->subscribeInstrument(exchange->getInstrumentByName(security->GetSymbol()))) {
		Log::Error(TRADER_ENYX_LOG_PREFFIX "failed to subscribe \"%1%\".", security->GetFullSymbol());
		return;
	}
	m_handler->Subscribe(security);
}
