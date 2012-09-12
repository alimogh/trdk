/**************************************************************************
 *   Created: 2012/09/11 18:54:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "MarketData.hpp"
#include "Core/Security.hpp"
#include "Common/IniFile.hpp"
#include "NasdaqFeedHandler.hpp"

namespace fs = boost::filesystem;
using namespace Trader::Interaction::Enyx;

MarketData::MarketData() {
	//...//
}

MarketData::~MarketData() {
	//...///
}

void MarketData::Connect(const IniFile &ini, const std::string &section) {

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
            port.setInterfaceForLane(ENYXMD_LANE_A, "enyxnet0");
            port.setInterfaceForLane(ENYXMD_LANE_B, "enyxnet0");
            enyx.reset(&hardware);
        } catch (const UnsuportedFeedException &ex) {
			Log::Error(
				TRADER_ENYX_LOG_PREFFIX "failed to make hardware connection: \"%1%\".",
				ex.what());
			throw ConnectError();
        }
	}

    enyx->setFirstStreamLane(ENYXMD_LANE_A);
    enyx->setSecondStreamLane(ENYXMD_LANE_B);
    enyx->setInstrumentFiltering(true);
    enyx->setEmptyPacketDrop(true);

	NXFeedHandler *nxOrderManager = new NasdaqFeedHandler;
    enyx->addHandler(*nxOrderManager);

	m_enyx.reset(enyx.release());
	Log::Info(TRADER_ENYX_LOG_PREFFIX "connected.");

}

void MarketData::Start() {
	Log::Info(TRADER_ENYX_LOG_PREFFIX "starting...");
	Assert(m_enyx);
	m_enyx->start();
	Log::Info(TRADER_ENYX_LOG_PREFFIX "started.");
}

bool MarketData::IsSupported(const Security &security) const {
	return Dictionary::isExchangeSupported(security.GetPrimaryExchange());
}

void MarketData::CheckSupport(const Security &security) const {
	if (!IsSupported(security)) {
		boost::format message("Exchange \"%1%\" not supported by market data source");
		message % security.GetPrimaryExchange();
		throw Error(message.str().c_str());
	}
}

void MarketData::SubscribeToMarketDataLevel1(boost::shared_ptr<Security> security) const {
	Log::Info(TRADER_ENYX_LOG_PREFFIX "subscribing \"%1%\"...", security->GetFullSymbol());
	CheckSupport(*security);
	const auto exchange = Dictionary::getExchangeByName(security->GetPrimaryExchange());
	m_enyx->subscribeInstrument(exchange->getInstrumentByName(security->GetSymbol()));
}

void MarketData::SubscribeToMarketDataLevel2(boost::shared_ptr<Security>) const {
// 	const auto exchange = Dictionary::getExchangeByName(security->GetPrimaryExchange());
//     m_enyx->subscribeInstrument(exchange->getInstrumentByName(security->GetSymbol()));

}




