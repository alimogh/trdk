/**************************************************************************
 *   Created: 2016/10/30 18:09:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TransaqMaketDataSource.hpp"
#include "TransaqConnectorContext.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Transaq;

namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

Transaq::MarketDataSource::MarketDataSource(
		size_t index,
		Context &context,
		const std::string &tag)
	: trdk::MarketDataSource(index, context, tag)
	, MarketDataSourceConnector(GetContext(), GetLog()) {
	//...//
}

Transaq::MarketDataSource::~MarketDataSource() {
	// Each object, that implements CreateNewSecurityObject should wait for
	// log flushing before destroying objects:
	m_securities.clear();
	GetTradingLog().WaitForFlush();
}

void Transaq::MarketDataSource::Connect(const IniSectionRef &conf) {
	Init();
	try {
		Connector::Connect(conf);
	} catch (const Connector::ConnectException &ex) {
		throw ConnectException(ex.what());
	}
} 

void Transaq::MarketDataSource::SubscribeToSecurities() {

	GetLog().Info(
		"Sending market data subscription request for %1% securities...",
		m_securities.size());

	std::vector<std::pair<std::string, std::string>> list;
	for (const auto &security: m_securities) {
		GetLog().Debug(
			"Sending market data subscription request for %1%...",
			*security.second);
		list.emplace_back(security.first);
	}

	try {
		SendMarketDataSubscriptionRequest(list);
	} catch (const Connector::Exception &ex) {
		GetLog().Error(
			"Failed to send market data subscription request: \"%1%\".",
			ex.what());
		throw Exception("Failed to send market data subscription request");
	}

	GetLog().Info("Market data subscription request sent.");

}

trdk::Security & Transaq::MarketDataSource::CreateNewSecurityObject(
		const Symbol &symbol) {

	const auto security = boost::make_shared<Transaq::Security>(
		GetContext(),
		symbol,
		*this,
		Transaq::Security::SupportedLevel1Types()
			.set(LEVEL1_TICK_LAST_PRICE)
			.set(LEVEL1_TICK_LAST_QTY)
			.set(LEVEL1_TICK_BID_PRICE)
			.set(LEVEL1_TICK_BID_QTY)
			.set(LEVEL1_TICK_ASK_PRICE)
			.set(LEVEL1_TICK_ASK_QTY)
			.set(LEVEL1_TICK_TRADING_VOLUME));

	security->SetTradingSessionState(GetContext().GetCurrentTime(), true);
	security->SetOnline(GetContext().GetCurrentTime(), true);

	const auto &key = std::make_pair(symbol.GetExchange(), symbol.GetSymbol());
	Verify(m_securities.emplace(key, security).second);

	return *security;

}

void Transaq::MarketDataSource::OnNewTick(
		const pt::ptime &time,
		const std::string &board,
		const std::string &symbol,
		double price,
		const Qty &qty,
		const Milestones &delayMeasurement) {
	auto &security = GetSecurity(board, symbol);
	security.AddTrade(
		time,
		security.ScalePrice(price),
		qty,
		delayMeasurement,
		true);
}

void Transaq::MarketDataSource::OnLevel1Update(
		const std::string &board,
		const std::string &symbol,
		boost::optional<double> &&bidPrice,
		boost::optional<double> &&bidQty,
		boost::optional<double> &&askPrice,
		boost::optional<double> &&askQty,
		const Milestones &delayMeasurement) {
	Assert(bidPrice || bidQty || askPrice || askQty);
	auto &security = GetSecurity(board, symbol);
	std::vector<Level1TickValue> ticks;
	ticks.reserve(4);
	if (bidPrice) {
		ticks.emplace_back(
			Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(
				security.ScalePrice(*bidPrice)));
	}
	if (bidQty) {
		ticks.emplace_back(
			Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(
				security.ScalePrice(*bidQty)));
	}
	if (askPrice) {
		ticks.emplace_back(
			Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(
				security.ScalePrice(*askPrice)));
	}
	if (askQty) {
		ticks.emplace_back(
			Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(
				security.ScalePrice(*askQty)));
	}
	Assert(!ticks.empty());
	security.SetLevel1(pt::not_a_date_time, ticks, delayMeasurement);
}

Transaq::Security & Transaq::MarketDataSource::GetSecurity(
		const std::string &board,
		const std::string &symbol) {
	const auto &result = m_securities.find(std::make_pair(board, symbol));
	if (result == m_securities.cend()) {
		GetLog().Error("Unknown security \"%1%/%2%\".", board, symbol);
		throw Exception("Unknown security");
	}
	return *result->second;
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::MarketDataSource> CreateMarketDataSource(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &) {
	
	class MarketDataSource : public Transaq::MarketDataSource {
	public:
		MarketDataSource(
				size_t index,
				Context &context,
				const std::string &tag)
			: Transaq::MarketDataSource(index, context, tag)
			, m_connectorContext(GetContext(), GetLog()) {
			//...//
		}
		virtual ~MarketDataSource() {
			//...//
		}
	protected:
		virtual ConnectorContext & GetConnectorContext() override {
			return m_connectorContext;
		}
	private:
		ConnectorContext m_connectorContext;
	};

	return boost::make_shared<MarketDataSource>(
		index,
		context,
		tag);

}

////////////////////////////////////////////////////////////////////////////////
