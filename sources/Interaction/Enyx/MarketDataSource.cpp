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

///////////////////////////////////////////////////////////////////////////////

namespace {
	void FirstLimitUpdateCallback(
				std::string instrument,
				uint64_t price,
				uint32_t quantity,
				bool buy_nSell,
				void *callBackArg) {
		try {
			boost::trim(instrument);
			if (price > 1000000) {
				price /= 1000000;
			} else {
				price *= 100;
			}
			static_cast<FirstLimitUpdateHandler *>(callBackArg)->HandleUpdate(
				instrument,
				price,
				quantity,
				buy_nSell);
		} catch (...) {
			AssertFailNoException();
		}
	}
}

MarketDataSource::OrderHandler::OrderHandler()
		: manager(&FirstLimitUpdateCallback, &handler) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

namespace {

	struct IniLaneExtractor {

		const IniFile &ini;
		const std::string &section;

		explicit IniLaneExtractor(const IniFile &ini, const std::string &section)
				: ini(ini),
				section(section) {
			//...//
		}

		enyxmd_stream_lane Get(const unsigned int laneNumb) const {
			const std::string key
				= (boost::format("stream_lane_%1%") % laneNumb).str();
			const std::string val = ini.ReadKey(section, key, false);
			enyxmd_stream_lane result = ENYXMD_LANE_EMPTY;
			if (boost::iequals(val, "EMPTY")) {
				result = ENYXMD_LANE_EMPTY;
			} else if (boost::iequals(val, "A")) {
				result = ENYXMD_LANE_A;
			} else if (boost::iequals(val, "B")) {
				result = ENYXMD_LANE_B;
			} else if (boost::iequals(val, "C")) {
				result = ENYXMD_LANE_C;
			} else if (boost::iequals(val, "D")) {
				result = ENYXMD_LANE_D;
			} else {
				Log::Error(
					TRADER_ENYX_LOG_PREFFIX "failed to get Enyx stream lane for %1% = \"%2%\".",
					key,
					val);
				throw IniFile::KeyFormatError("Failed to get Enyx stream lane");
			}
			boost::format message(
				TRADER_ENYX_LOG_PREFFIX "using %1% Lane (%2% = \"%3%\").");
			switch (result) {
				case ENYXMD_LANE_EMPTY:
					message % "No";
					break;
				case ENYXMD_LANE_A:
					message % "A";
					break;
				case ENYXMD_LANE_B:
					message % "B";
					break;
				case ENYXMD_LANE_C:
					message % "C";
					break;
				case ENYXMD_LANE_D:
					message % "D";
					break;
				default:
					AssertFail("Unknown Enyx Stream Lane.");
			}
			message % key % val;
			Log::Info(message.str().c_str());
			return result;
		}

	};

}

MarketDataSource::MarketDataSource(const IniFile &ini, const std::string &section) {

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
    Log::Info(
		TRADER_ENYX_LOG_PREFFIX "building Instrument List from \"%1%\".",
		totalViewNasdaqFeedSupport->getListingFilePath());

    totalViewNasdaqFeedSupport->updateInstruments();

	if (ini.IsKeyExist(section, "pcap")) {
		const fs::path pcapFilePath = ini.ReadKey(section, "pcap", true);
		Log::Info(TRADER_ENYX_LOG_PREFFIX "loading PCAP-file %1%...", pcapFilePath);
        try {
			m_enyx.reset(
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
			IniLaneExtractor extractor(ini, section);
            EnyxPort &port = hardware.getPortConfiguration();
            port.setInterfaceForLane(extractor.Get(1), "enyxnet0");
            port.setInterfaceForLane(extractor.Get(2), "enyxnet0");
            m_enyx.reset(&hardware);
        } catch (const UnsuportedFeedException &ex) {
			Log::Error(
				TRADER_ENYX_LOG_PREFFIX "failed to make hardware connection: \"%1%\".",
				ex.what());
			throw ConnectError();
        }
	}

	{
		IniLaneExtractor extractor(ini, section);
		m_enyx->setFirstStreamLane(extractor.Get(1));
		m_enyx->setSecondStreamLane(extractor.Get(2));
	}

	m_enyx->setInstrumentFiltering(true);
	m_enyx->setEmptyPacketDrop(true);

	const bool isFeedHandlerOn = ini.ReadBoolKey(section, "feed_handler");
	const bool isOrderHandlerOn = ini.ReadBoolKey(section, "order_handler");
	Log::Info(
		TRADER_ENYX_LOG_PREFFIX "feed_handler = %1%; order_handler = %2%;",
		isFeedHandlerOn ? "yes" : "no",
		isOrderHandlerOn ? "yes" : "no");
	if (!isFeedHandlerOn && !isOrderHandlerOn) {
		Log::Error(TRADER_ENYX_LOG_PREFFIX "failed to init handler: no handler set.");
		throw ConnectError();
	}

	if (isFeedHandlerOn) {
		m_feedHandler.reset(new FeedHandler(!isOrderHandlerOn));
		if (ini.ReadBoolKey(section, "raw_log")) {
			m_feedHandler->EnableRawLog();
		}
		m_enyx->addHandler(*m_feedHandler);
	}

	if (isOrderHandlerOn) {
		m_orderHandler.reset(new OrderHandler);
		m_enyx->addHandler(m_orderHandler->manager);
	}

}

MarketDataSource::~MarketDataSource() {
	//...///
}

void MarketDataSource::Connect() {
	Log::Info(TRADER_ENYX_LOG_PREFFIX "connecting...");
	m_enyx->start();
	Log::Info(TRADER_ENYX_LOG_PREFFIX "connected.");
}

bool MarketDataSource::IsSupported(const Trader::Security &security) const {
	return
		security.GetPrimaryExchange() == "NASDAQ-US"
		&& Dictionary::isExchangeSupported(security.GetPrimaryExchange());
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

void MarketDataSource::Subscribe(const boost::shared_ptr<Security> &security) const {

	Assert(m_orderHandler || m_feedHandler);

	Log::Info(
		TRADER_ENYX_LOG_PREFFIX "subscribing \"%1%\"...",
		security->GetFullSymbol());

	CheckSupport(*security);

	const auto exchange = Dictionary::getExchangeByName(security->GetPrimaryExchange());
	Assert(exchange);
	if (!m_enyx->subscribeInstrument(exchange->getInstrumentByName(security->GetSymbol()))) {
		Log::Error(
			TRADER_ENYX_LOG_PREFFIX "failed to subscribe \"%1%\" (%2%).",
			security->GetSymbol(),
			security->GetPrimaryExchange());
		throw Error("Failed to subscribe to Enyx Market Data");
	}

	if (m_orderHandler) {
		m_orderHandler->manager.setSubscribedInstruments(
			m_enyx->getSetSubscribedInstrument());
		m_orderHandler->handler.Register(security);
	}

	if (m_feedHandler) {
		m_feedHandler->Subscribe(security);
	}

}
