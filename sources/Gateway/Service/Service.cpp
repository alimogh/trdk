/**************************************************************************
 *   Created: 2012/09/19 23:46:49
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Service.hpp"
#include "Core/Security.hpp"
#include "Core/Position.hpp"

using namespace Trader;
using namespace Trader::Lib;
using namespace Trader::Gateway;
namespace pt = boost::posix_time;

Service::Service(
			const std::string &tag,
			const Observer::NotifyList &notifyList,
			boost::shared_ptr<Trader::TradeSystem> tradeSystem,
			const IniFile &ini,
			const std::string &section)
		: Observer(tag, notifyList, tradeSystem),
		m_stopFlag(false) {

	m_port = ini.ReadTypedKey<unsigned short>(section, "port");
			
	try {
		soap_init2(&m_soap, SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE);
		m_soap.user = this;
		m_soap.max_keep_alive = 1000;
		StartSoapDispatcherThread();
	} catch (...) {
		soap_done(&m_soap);
		throw;
	}

}

Service::~Service() {
	Interlocking::Exchange(m_stopFlag, true);
	soap_done(&m_soap);
	{
		boost::mutex::scoped_lock lock(m_connectionRemoveMutex);
		const Connections::const_iterator end(m_connections.end());
		for (Connections::const_iterator i(m_connections.begin()); i != end; ++i) {
			soap_done(*i);
		}
	}
	m_threads.join_all();
}

const std::string & Service::GetName() const {
	static const std::string name = "Gateway";
	return name;
}

void Service::LogSoapError() const {
	std::ostringstream oss;
	soap_stream_fault(&m_soap, oss);
	std::string soapError(
		boost::regex_replace(
			oss.str(),
			boost::regex("[\n\t\r]"),
			" ",
			boost::match_default | boost::format_all));
	boost::trim(soapError);
	if (soapError.empty()) {
		soapError = "Unknown SOAP error.";
	}
	Log::Error(TRADER_GATEWAY_LOG_PREFFIX "%1%", soapError);
}

void Service::SoapServeThread(::soap *soap) {

	class Cleaner : private boost::noncopyable {
	public:
		Cleaner(::soap &soap, Connections &connections, ConnectionRemoveMutex &connectionsMutex)
				: m_soap(soap),
				m_connections(connections),
				m_connectionsMutex(connectionsMutex) {
			//...//
		}
		~Cleaner() {
			const auto ip = m_soap.ip;
			soap_destroy(&m_soap);
 			soap_end(&m_soap);
			{
				const ConnectionRemoveLock lock(m_connectionsMutex);
				const Connections::iterator pos(m_connections.find(&m_soap));
				if (pos != m_connections.end()) {
 					soap_done(&m_soap);
 					m_connections.erase(pos);
				}
			}
			free(&m_soap);
			Log::Info(
				TRADER_GATEWAY_LOG_PREFFIX "closed connection from %d.%d.%d.%d.",
				(ip >> 24) & 0xFF,
				(ip >> 16) & 0xFF,
				(ip >> 8) & 0xFF,
				ip & 0xFF);
		}
	private:
		::soap &m_soap;
		Connections &m_connections;
		ConnectionRemoveMutex &m_connectionsMutex;
	} cleaner(*soap, m_connections, m_connectionRemoveMutex);

	const int serveResult = soap_serve(soap);
	if (	serveResult != SOAP_OK
			&& serveResult != SOAP_EOF
			/*&& serveResult != SOAP_NO_METHOD*/) {
		LogSoapError();
	}

}

void Service::HandleSoapRequest() {

	SOAP_SOCKET acceptedSocket = soap_accept(&m_soap);
	if (m_stopFlag) {
		return;
	}

	if (acceptedSocket < 0) {
		LogSoapError();
		return;
	}

	Log::Info(
		TRADER_GATEWAY_LOG_PREFFIX "accepted connection from %d.%d.%d.%d.",
		(m_soap.ip >> 24) & 0xFF,
		(m_soap.ip >> 16) & 0xFF,
		(m_soap.ip >> 8) & 0xFF,
		m_soap.ip & 0xFF);

	soap *connection = soap_copy(&m_soap);
	m_connections.insert(connection);
	m_threads.create_thread(boost::bind(&Service::SoapServeThread, this, connection));

 	soap_destroy(&m_soap);
 	soap_end(&m_soap);

}

void Service::StartSoapDispatcherThread() {

	for (auto i = 0; ; ++i) {

		const char *const serviceHost = NULL;
		SOAP_SOCKET masterSocket = soap_bind(&m_soap, serviceHost, m_port, 100);
		if (!soap_valid_socket(masterSocket)) {
			if (m_soap.error == 28 && i < 12) {
				Log::Warn("SOAP port %1% is busy, waiting...", m_port);
				boost::this_thread::sleep(pt::seconds(5));
				continue;
			}
			LogSoapError();
			throw Exception("Failed to start Gateway SOAP Server");
		}

		Log::Info(TRADER_GATEWAY_LOG_PREFFIX "started at port %1%.", m_port);

		{
			sockaddr_in hostInfo;
			socklen_t hostInfoSize = sizeof(hostInfo);
			if (getsockname(masterSocket, reinterpret_cast<sockaddr *>(&hostInfo), &hostInfoSize)) {
				const Trader::Lib::Error error(GetLastError());
				Log::Error(
					TRADER_GATEWAY_LOG_PREFFIX "failed to get to get local network port info: %1%.",
					error);
				throw Exception("Failed to start Gateway SOAP Server");
			}
			m_soap.port = ntohs(hostInfo.sin_port);
		}

		m_threads.create_thread(boost::bind(&Service::SoapDispatcherThread, this));

		break;

	}

}

void Service::SoapDispatcherThread() {
	while (!m_stopFlag) {
		HandleSoapRequest();
	}
}

namespace {

	time_t ConvertPosixTimeToTimeT(
				const boost::posix_time::ptime &posixTime) {
		namespace pt = boost::posix_time;
		Assert(!posixTime.is_special());
		static const pt::ptime timeTEpoch(boost::gregorian::date(1970, 1, 1));
		Assert(!(posixTime < timeTEpoch));
		if (posixTime < timeTEpoch) {
			return 0;
		}
		const pt::time_duration durationFromTEpoch(posixTime - timeTEpoch);
		return static_cast<time_t>(durationFromTEpoch.total_seconds());
	}

}

void Service::OnUpdate(
			const Trader::Security &security,
			const boost::posix_time::ptime &time,
			Trader::Security::ScaledPrice price,
			Trader::Security::Qty qty,
			bool isBuy) {
	boost::shared_ptr<trader__Trade> update(new trader__Trade);
	const auto date = time - time.time_of_day();
	update->time.date = ConvertPosixTimeToTimeT(date);
	update->time.time = (time - date).total_milliseconds();
	update->param.price = price;
	update->param.qty = qty;
	update->isBuy = isBuy;
	const TradesCacheLock lock(m_tradesCacheMutex);
	auto &cache = m_tradesCache[security.GetSymbol()];
	cache.push_back(update);
}

void Service::GetSecurityList(std::list<trader__Security> &result) {
	std::list<trader__Security> resultTmp;
	foreach (const auto &serviceSecurity, GetNotifyList()) {
		trader__Security security;
		static_assert(
			sizeof(decltype(security.id)) >= sizeof(void *),
			"trader__Security::id too small.");
		typedef decltype(security.id) Id;
		security.id = Id(serviceSecurity.get());
		security.symbol = serviceSecurity->GetSymbol();
		security.scale = serviceSecurity->GetPriceScale();
		resultTmp.push_back(security);
	}
	resultTmp.swap(result);
}

void Service::GetLastTrades(
					const std::string &symbol,
					const std::string &exchange,
					trader__TradeList &result) {
	std::list<trader__Trade> resultTmp;
	std::list<boost::shared_ptr<trader__Trade>> cache;
	if (boost::iequals(exchange, "nasdaq")) {
		const TradesCacheLock lock(m_tradesCacheMutex);
		const auto it = m_tradesCache.find(symbol);
		if (it != m_tradesCache.end()) {
			it->second.swap(cache);
		}
	}
	foreach (const auto &update, cache) {
		resultTmp.push_back(*update);
	}
	resultTmp.swap(result);
}

const Trader::Security & Service::FindSecurity(const std::string &symbol) const {
	return const_cast<Service *>(this)->FindSecurity(symbol);
}

Trader::Security & Service::FindSecurity(const std::string &symbol) {
	foreach (auto &security, GetNotifyList()) {
		if (boost::iequals(security->GetSymbol(), symbol)) {
			return *security;
		}
	}
	boost::format error("Failed to find security \"%1%\" in notify list");
	error % symbol;
	throw UnknownSecurityError(error.str().c_str());
}

void Service::GetParams(
			const std::string &symbol,
			const std::string &exchange,
			trader__ExchangeBook &result) {
	if (!boost::iequals(exchange, "nasdaq")) {
		result = trader__ExchangeBook();
		return;
	}
	try {
		const Security &security = FindSecurity(symbol);
		result.params1.ask.price = security.GetAskPriceScaled(1);
		result.params1.ask.qty = security.GetAskQty(1);
		result.params1.bid.price = security.GetBidPriceScaled(1);
		result.params1.bid.qty = security.GetBidQty(1);
		result.params2.ask.price = security.GetAskPriceScaled(2);
		result.params2.ask.qty = security.GetAskQty(2);
		result.params2.bid.price = security.GetBidPriceScaled(2);
		result.params2.bid.qty = security.GetBidQty(2);
	} catch (const UnknownSecurityError &) {
		return;
	}
}

void Service::GetCommonParams(
			const std::string &symbol,
			trader__CommonParams &result) {
	try {
		const Security &security = FindSecurity(symbol);
		result.last.price = security.GetLastPriceScaled();
		result.last.qty = security.GetLastQty();
		result.best.ask.price = security.GetAskPriceScaled(1);
		result.best.ask.qty = security.GetAskQty(1);
		result.best.bid.price = security.GetBidPriceScaled(1);
		result.best.bid.qty = security.GetBidQty(1);
		result.volumeTraded = security.GetTradedVolume();
	} catch (const UnknownSecurityError &) {
		return;
	}
}

void Service::OrderBuy(
			const std::string &symbol,
			const std::string &venue,
			Security::ScaledPrice price,
			Security::Qty qty,
			std::string &resultMessage) {
	try {
		Security &security = FindSecurity(symbol);
		security.SetExchange(venue);
		ShortPosition &shortPosition = GetShortPosition(security);
		if (shortPosition.GetPlanedQty() < qty) {
			LongPosition &longPosition = GetLongPosition(security);
			longPosition.IncreasePlanedQty(qty);
			longPosition.Open(price);
		} else {
			shortPosition.Close(Position::CLOSE_TYPE_NONE, price, qty);
		}
		boost::format message("Buy Order for %1% (price %2%, quantity %3%) successfully sent.");
		message % symbol % security.DescalePrice(price) % qty;
		resultMessage = message.str();
	} catch (const UnknownSecurityError &) {
		boost::format message("Failed to send Buy Order for %1% - unknown instrument.");
		message % symbol;
		resultMessage = message.str();
	}
}

void Service::OrderBuyMkt(
			const std::string &symbol,
			const std::string &venue,
			Security::Qty qty,
			std::string &resultMessage) {
	try {
		Security &security = FindSecurity(symbol);
		security.SetExchange(venue);
		ShortPosition &shortPosition = GetShortPosition(security);
		if (shortPosition.GetPlanedQty() < qty) {
			LongPosition &longPosition = GetLongPosition(security);
			longPosition.IncreasePlanedQty(qty);
			longPosition.OpenAtMarketPrice();
		} else {
			shortPosition.CloseAtMarketPrice(Position::CLOSE_TYPE_NONE, qty);
		}
		boost::format message("Buy Market Order for %1% (quantity %2%) successfully sent.");
		message % symbol % qty;
		resultMessage = message.str();
	} catch (const UnknownSecurityError &) {
		boost::format message("Failed to send Buy Market Order for %1% - unknown instrument.");
		message % symbol;
		resultMessage = message.str();
	}
}

void Service::OrderSell(
			const std::string &symbol,
			const std::string &venue,
			Security::ScaledPrice price,
			Security::Qty qty,
			std::string &resultMessage) {
	try {
		Security &security = FindSecurity(symbol);
		security.SetExchange(venue);
		LongPosition &longPosition = GetLongPosition(security);
		if (longPosition.GetPlanedQty() < qty) {
			ShortPosition &shortPosition = GetShortPosition(security);
			shortPosition.IncreasePlanedQty(qty);
			shortPosition.Open(price);
		} else {
			longPosition.Close(Position::CLOSE_TYPE_NONE, price, qty);
		}
		boost::format message("Sell Order for %1% (price %2%, quantity %3%) successfully sent.");
		message % symbol % security.DescalePrice(price) % qty;
		resultMessage = message.str();
	} catch (const UnknownSecurityError &) {
		boost::format message("Failed to send Sell Order for %1% - unknown instrument.");
		message % symbol;
		resultMessage = message.str();
	}
}

void Service::OrderSellMkt(
			const std::string &symbol,
			const std::string &venue,
			Security::Qty qty,
			std::string &resultMessage) {
	try {
		Security &security = FindSecurity(symbol);
		security.SetExchange(venue);
		LongPosition &longPosition = GetLongPosition(security);
		if (longPosition.GetPlanedQty() < qty) {
			ShortPosition &shortPosition = GetShortPosition(security);
			shortPosition.IncreasePlanedQty(qty);
			shortPosition.OpenAtMarketPrice();
		} else {
			longPosition.CloseAtMarketPrice(Position::CLOSE_TYPE_NONE, qty);
		}
		boost::format message("Sell Market Order for %1% (quantity %2%) successfully sent.");
		message % symbol % qty;
		resultMessage = message.str();
	} catch (const UnknownSecurityError &) {
		boost::format message("Failed to send Sell Market Order for %1% - unknown instrument.");
		message % symbol;
		resultMessage = message.str();
	}
}

ShortPosition & Service::GetShortPosition(Trader::Security &security) {
	boost::shared_ptr<ShortPosition> &result = m_positions[&security].first;
	if (!result || result->IsClosed()) {
		result.reset(new ShortPosition(security.shared_from_this()));
	}
	return *result;
}

LongPosition & Service::GetLongPosition(Trader::Security &security) {
	boost::shared_ptr<LongPosition> &result = m_positions[&security].second;
	if (!result || result->IsClosed()) {
		result.reset(new LongPosition(security.shared_from_this()));
	}
	return *result;
}

void Service::GetPositionInfo(
			const std::string &symbol,
			trader__Position &result)
		const {
	try {
		const Security &security = FindSecurity(symbol);
		const Positions::const_iterator positionIt = m_positions.find(&security);
		if (positionIt == m_positions.end()) {
			result = trader__Position();
			return;
		}
		const auto position = positionIt->second;
		const auto shortSideQty = !position.first
			?	0
			:	position.first->GetActiveQty();
		const auto longSideQty = !position.second
			?	0
			:	position.second->GetActiveQty();
		const auto qty = longSideQty - abs(shortSideQty);
		if (qty == 0) {
			result = trader__Position();
			return;
		}
		result.side = qty > 0
			?	1
			:	-1;
		result.price = qty < 0 || longSideQty <= 0
			?	position.first->GetOpenPrice()
			:	position.second->GetOpenPrice();
		result.qty = abs(qty);
	} catch (const UnknownSecurityError &) {
		Log::Error(
			"Failed to send position info: unknown instrument \"%1%\".",
			symbol);
	}
}
