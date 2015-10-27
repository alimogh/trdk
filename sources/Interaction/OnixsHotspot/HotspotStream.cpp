/**************************************************************************
 *   Created: 2014/10/16 00:50:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "HotspotSecurity.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace hsi = OnixS::HotspotItch;

namespace trdk { namespace Interaction { namespace OnixsHotspot {

	class HotspotStream
			: public trdk::MarketDataSource,
			public hsi::Listener {

	public:

		explicit HotspotStream(
					size_t index,
					Context &context,
					const std::string &tag,
					const IniSectionRef &conf)
				: MarketDataSource(index, context, tag),
				m_handler(GetHandlerOptions(conf)) {
			m_handler.registerListener(this);
		}

		virtual ~HotspotStream() {
			try {
				m_handler.unsubscribeMarketDataForAllCurrencyPairs();
				m_handler.subscribeTickerForAllCurrencyPairs();
				m_handler.disconnect();
			} catch (...) {
				EventsLog::BroadcastUnhandledException(
					__FUNCTION__,
					__FILE__,
					__LINE__);
			}
			// Each object, that implements CreateNewSecurityObject should waite for
			// log flushing before destroying objects:
			GetTradingLog().WaitForFlush();
		}

	public:

		virtual void Connect(const IniSectionRef &conf) {

			if (m_handler.connected()) {
				return;
			}

			m_connectionOptions.host = conf.ReadKey("server_host");
			m_connectionOptions.port = conf.ReadTypedKey<int>("server_port");
			m_connectionOptions.login = conf.ReadKey("login");
			m_connectionOptions.password = conf.ReadKey("password");

			Connect();

		}

		virtual void SubscribeToSecurities() {
			GetLog().Info(
				"Sending market data requests for %1% securities...",
				m_securities.size());
			foreach (const auto &security, m_securities) {
				const auto &symbol = security.second->GetSymbol().GetSymbol();
				GetLog().Info("Sending market data request for %1%...", symbol);
				m_handler.subscribeMarketData(symbol);
			}
		}

	public:

		/// Is called when the error condition is detected.
		virtual void onError(const std::string &errorDescription) {
			GetLog().Error("Session error: \"%1%\".", errorDescription);
		}

		/// Is called when Handler is disconnected.
		virtual void onDisconnect(const std::string &disconnectReason) {
			GetLog().Info(
				"Disconnected with reason: \"%1%\". Reconnecting...",
				disconnectReason);
			Connect();
		}

		/// Is called when the Login Accepted message is received.
		virtual void onLoginAccepted(int visibilityBitMask) {
			GetLog().Info(
				"Login request server accepted (mask: %1%).",
				visibilityBitMask);
		}

		/// Is called when the Login Rejected message is received.
		virtual void onLoginRejected(const std::string &rejectReason) {
			GetLog().Error(
				"Login request to server REJECTED with reason: \"%1%\".",
				rejectReason);
		}

		/// Is called when the Instrument Directory message is received.
		virtual void onInstrumentDirectory(const hsi::CurrencyPairs &) {
			//...//
		}

		/// Is called when the Market Snapshot message is received.
		virtual void onMarketSnapshot(const hsi::MarketSnapshotMessage &) {
			//...//
		}

		/// Is called when the New Order message is received.
		virtual void onNewOrder(const hsi::NewOrderMessage &) {
			//...//
		}

		/// Is called when the Modify Order message is received.
		virtual void onModifyOrder(const hsi::ModifyOrderMessage &) {
			//...//
		}

		/// Is called when the Cancel Order message is received.
		virtual void onCancelOrder(const hsi::CancelOrderMessage &) {
			//...//
		}

		/// Is called when the Ticker message is received.
		virtual void onTicker(const hsi::TickerMessage &) {
			//...//
		}

		/// Is called when the Currency pair price levels updated.
		virtual void onCurrencyPairUpdated(const hsi::CurrencyPair &pair) {

			const auto &timeMeasurement
				= GetContext().StartStrategyTimeMeasurement();
			const auto &now
				= boost::posix_time::microsec_clock::universal_time();

			const auto &securityPos = m_securities.find(pair.pair());
			if (securityPos == m_securities.end()) {
				return;
			}
			HotspotSecurity &security = *securityPos->second;
			
			pair.getAskPrices(&security.GetAsksCache());
			pair.getBidPrices(&security.GetBidsCache());

			security.Flush(now, timeMeasurement);
			
		}

	protected:

		virtual trdk::Security & CreateNewSecurityObject(const Symbol &symbol) {
			boost::shared_ptr<HotspotSecurity> result(
				new HotspotSecurity(GetContext(), symbol, *this));
			auto securities(m_securities);
			Assert(
				securities.find(result->GetSymbol().GetSymbol())
					== securities.end());
			securities[result->GetSymbol().GetSymbol()] = result;
			securities.swap(const_cast<HotspotStream *>(this)->m_securities);
			return *result;
		}

	private:

		static hsi::HandlerOptions GetHandlerOptions(
					const Lib::IniSectionRef &) {
			hsi::HandlerOptions result;
			result.logSettings = hsi::LogSettings::Default;
			result.feedProvidesMinQtyLotSize = true;
#			ifdef DEV_VER
				result.logLevel = hsi::LogLevel::Debug;
#			else
				result.logLevel = hsi::LogLevel::Info;
#			endif
			return result;
		}

		void Connect() {

			if (m_handler.connected()) {
				return;
			}

			GetLog().Info(
				"Connecting to server at \"%1%:%2%\""
					" as \"%3%\" (with password)...",
				m_connectionOptions.host,
				m_connectionOptions.port,
				m_connectionOptions.login);

			try {
				m_handler.connect(
					m_connectionOptions.host,
					m_connectionOptions.port,
					m_connectionOptions.login,
					m_connectionOptions.password);
			} catch (const std::exception &ex) {
				throw ConnectError(ex.what());
			}

		}

	private:

		hsi::Handler m_handler;
		std::map<std::string, boost::shared_ptr<HotspotSecurity>> m_securities;

		struct {
			std::string host;
			int port;
			std::string login;
			std::string password;
		} m_connectionOptions;

	};

} } }

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXSHOTSPOT_API
boost::shared_ptr<MarketDataSource>
CreateStream(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	return boost::shared_ptr<MarketDataSource>(
		new trdk::Interaction::OnixsHotspot::HotspotStream(
			index,
			context,
			tag,
			configuration));
}

////////////////////////////////////////////////////////////////////////////////
