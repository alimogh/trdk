/**************************************************************************
 *   Created: May 26, 2012 9:44:52 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "IbTradeSystem.hpp"
#include "IbClient.hpp"
#include "Core/Security.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::InteractiveBrokers;

namespace ib = trdk::Interaction::InteractiveBrokers;
namespace pt = boost::posix_time;


ib::TradeSystem::TradeSystem(
		const TradingMode &mode,
		size_t index,
		Context &context,
		const std::string &tag,
		const Lib::IniSectionRef &conf)
	: trdk::TradeSystem(mode, index, context, tag)
	, trdk::MarketDataSource(index, context, tag)
	, m_isTestSource(conf.ReadBoolKey("test_source", false)) {
	if (conf.ReadBoolKey("positions", false)) {
		m_positions.reset(new Positions);
	}
	{
		const auto &expirationCalendarFilePath = conf.ReadFileSystemPath(
			"expiration_calendar_csv",
			std::string());
		if (!expirationCalendarFilePath.empty()) {
			GetMdsLog().Debug(
				"Loading expiration calendar from %1%...",
				expirationCalendarFilePath);
			m_expirationCalendar.ReloadCsv(expirationCalendarFilePath);
			const ExpirationCalendar::Stat stat
				= m_expirationCalendar.CalcStat();
			const char *const message
				= "Expiration calendar has %1% symbols and %2% expirations.";
			stat.numberOfExpirations && stat.numberOfSymbols
				?	GetMdsLog().Info(
						message,
						stat.numberOfSymbols,
						stat.numberOfExpirations)
				:	GetMdsLog().Warn(
						message,
						stat.numberOfSymbols,
						stat.numberOfExpirations);
		}
	}
}

ib::TradeSystem::~TradeSystem() {
	//...//
}

void ib::TradeSystem::Connect(const IniSectionRef &settings) {
	// Implementation for trdk::MarketDataSource.
	if (IsConnected()) {
		return;
	}
	GetMdsLog().Info("Creating connection...");
	CreateConnection(settings);
}

bool ib::TradeSystem::IsConnected() const {
	return m_client ? true : false;
}

void ib::TradeSystem::CreateConnection(const IniSectionRef &settings) {

	Assert(!m_client);
	Assert(m_securities.empty());

	std::unique_ptr<Account> account;
	std::unique_ptr<Client> client(
		new Client(
			*this,
			settings.ReadBoolKey("no_history", false),
			settings.ReadTypedKey<int>("client_id", 0),
			settings.ReadKey("ip_address", "127.0.0.1"),
			settings.ReadTypedKey<unsigned short>("port", 7496)));

	if (settings.IsKeyExist("account")) {
		account.reset(new Account);
		client->SetAccount(settings.ReadKey("account", ""), *account);
	}

	client->Subscribe(
		[this](
				const OrderId &id,
				int permOrderId,
				const OrderStatus &status,
				const Qty &filled,
				const Qty &remaining,
				double lastFillPrice,
				Client::OrderCallbackList &callBackList) {
			TradeInfo tradeData = {};
			OrderStatusUpdateSlot callBack;
			{
				auto &index = m_placedOrders.get<ByOrder>();
				const OrdersWriteLock lock(m_ordersMutex);
				const auto pos = index.find(id);
				if (pos == index.end()) {
					/* GetTsLog().Debug(
						"Failed to find order by ID \"%1%\""
							" (status %2%, filled %3%, remaining %4%"
							", last price %5%). ",
						id,
						status,
						filled,
						remaining,
						lastFillPrice); */
					return;
				}
				callBack = pos->callback;
				switch (status) {
					default:
						return;
					case ORDER_STATUS_FILLED:
						AssertLt(0, filled);
						AssertGt(filled, pos->filled);
						tradeData.price
							= pos->security->ScalePrice(lastFillPrice);
						AssertLt(0, tradeData.price);
						tradeData.qty = filled - pos->filled;
						AssertLt(0, tradeData.qty);
						pos->UpdateFilled(filled);
						if (remaining == 0) {
							index.erase(pos);
						}
						break;
					case ORDER_STATUS_CANCELLED:
					case ORDER_STATUS_INACTIVE:
					case ORDER_STATUS_ERROR:
						index.erase(pos);
						break;
				}
			}
			if (!callBack) {
				return;
			}
			callBackList.push_back(
				[callBack, id, permOrderId, status, remaining, tradeData]() {
					const TradeInfo *const tradeDataPtr
						= !IsZero(tradeData.qty)
							?	&tradeData
							:	nullptr;	
					callBack(
						id,
						boost::lexical_cast<std::string>(permOrderId),
						status,
						remaining,
						tradeDataPtr);
				});
		});

	client->StartData();

	client.swap(m_client);
	account.swap(m_account);

}

void ib::TradeSystem::SubscribeToSecurities() {
	Assert(m_client);
	if (!m_client) {
		return;
	}
	while (!m_unsubscribedSecurities.empty()) {
		auto security = m_unsubscribedSecurities.front();
		m_unsubscribedSecurities.pop_front();
		m_securities.push_back(security);
		m_client->SubscribeToMarketData(*security);
	}
}

const ib::TradeSystem::Account & ib::TradeSystem::GetAccount() const {
	if (!m_account) {
		throw UnknownAccountError("Account not specified");
	}
	return *m_account;
}

trdk::TradeSystem::Position ib::TradeSystem::GetBrokerPostion(
		const std::string &account,
		const Symbol &symbol)
		const {
	if (!m_positions) {
		throw PositionError("Positions storage not enabled");
	}
	{
		const auto &index = m_positions->get<BySymbol>();
		const auto &key = boost::make_tuple(
			boost::cref(account),
			boost::cref(symbol.GetCurrency()),
			boost::cref(symbol.GetSymbol()));
		const PositionsReadLock lock(m_positionsMutex);
		const auto &it = index.find(key);
		if (it != index.end()) {
			return *it;
		}
	}
	return trdk::TradeSystem::Position();
}

void ib::TradeSystem::ForEachBrokerPostion(
		const std::string &account,
		const boost::function<
			bool (const trdk::TradeSystem::Position &)> &pred)
		const {
	if (!m_positions) {
		throw PositionError("Positions storage not enabled");
	}
	const auto &index = m_positions->get<ByAccount>();
	const PositionsReadLock lock(m_positionsMutex);
	const auto &end = index.upper_bound(account);
	for (auto it = index.lower_bound(account); it != end && pred(*it); ++it);
}

trdk::Security & ib::TradeSystem::CreateNewSecurityObject(
		const Symbol &symbol) {

	const auto &result = boost::make_shared<ib::Security>(
		GetContext(),
		symbol,
		*this,
		m_isTestSource);
	
	switch (symbol.GetSecurityType()) {
		case SECURITY_TYPE_FUTURES:
			{
				const auto &now = GetContext().GetCurrentTime();
				const auto &expiration = m_expirationCalendar.Find(symbol, now);
				if (!expiration) {
					boost::format error(
						"Failed to find expiration info for \"%1%\" and %2%");
					error % symbol % now;
					throw trdk::MarketDataSource::Error(error.str().c_str());
				}
				GetMdsLog().Info(
					"Current expiration date for \"%1%\": %2% (%3%%4%%5%).",
					symbol,
					expiration->expirationDate,
					symbol.GetSymbol(),
					char(expiration->code),
					expiration->year - 2010);
				result->SetExpiration(*expiration);
			}
			break;
	}

	m_unsubscribedSecurities.emplace_back(result);
	return *result;
}

void ib::TradeSystem::SendCancelOrder(const trdk::OrderId &orderId) {
	m_client->CancelOrder(orderId);
}

void ib::TradeSystem::SendCancelAllOrders(trdk::Security &security) {
	std::list<trdk::OrderId> ids;
	std::list<std::string> idsStr;
	{
		const OrdersReadLock lock(m_ordersMutex);
		const auto &index = m_placedOrders.get<BySymbol>();
		for (	auto i = index.find(&security)
				; i != index.end() && i->security == &security
				; ++i) {
			ids.push_back(i->id);
			idsStr.push_back(boost::lexical_cast<std::string>(i->id));
		}
	}
	std::for_each(
		ids.begin(),
		ids.end(),
		[this](trdk::OrderId orderId) {
			m_client->CancelOrder(orderId);
		});
}

trdk::OrderId ib::TradeSystem::SendSellAtMarketPrice(
		trdk::Security &security,
		const Currency &currency,
		const Qty &qty,
		const OrderParams &params,
		const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertEq(security.GetSymbol().GetCurrency(), currency);
	PlacedOrder order = {};
	order.id = m_client->PlaceSellOrder(security, qty, params);
	order.security = &security;
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}


trdk::OrderId ib::TradeSystem::SendSell(
		trdk::Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params,
		const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertEq(security.GetSymbol().GetCurrency(), currency);
	const auto rawPrice = security.DescalePrice(price);
	PlacedOrder order = {};
	order.id = m_client->PlaceSellOrder(security, qty, rawPrice, params);
	order.security = &security;
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::SendSellAtMarketPriceWithStopPrice(
		trdk::Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &stopPrice,
		const OrderParams &params,
		const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertEq(security.GetSymbol().GetCurrency(), currency);
	const auto rawStopPrice = security.DescalePrice(stopPrice);
	PlacedOrder order = {};
	order.id = m_client->PlaceSellOrderWithMarketPrice(
		security,
		qty,
		rawStopPrice,
		params);
	order.security = &security;
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::SendSellImmediatelyOrCancel(
		trdk::Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params,
		const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertEq(security.GetSymbol().GetCurrency(), currency);
	const double rawPrice = security.DescalePrice(price);
	const PlacedOrder order = {
		m_client->PlaceSellIocOrder(security, qty, rawPrice, params),
		&security,
		statusUpdateSlot
	};
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::SendBuyAtMarketPrice(
		trdk::Security &security,
		const Currency &currency,
		const Qty &qty,
		const OrderParams &params,
		const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertEq(security.GetSymbol().GetCurrency(), currency);
	PlacedOrder order = {};
	order.id = m_client->PlaceBuyOrder(security, qty, params);
	order.security = &security;
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::SendSellAtMarketPriceImmediatelyOrCancel(
		trdk::Security &,
		const trdk::Lib::Currency &,
		const trdk::Qty &,
		const trdk::OrderParams &,
		const OrderStatusUpdateSlot &) {
	throw MethodDoesNotImplementedError("Method is not implemented");
}

trdk::OrderId ib::TradeSystem::SendBuy(
		trdk::Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params,
		const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertEq(security.GetSymbol().GetCurrency(), currency);
	const auto rawPrice = security.DescalePrice(price);
	PlacedOrder order = {};
	order.id = m_client->PlaceBuyOrder(security, qty, rawPrice, params);
	order.security = &security;
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::SendBuyAtMarketPriceWithStopPrice(
		trdk::Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &stopPrice,
		const OrderParams &params,
		const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertEq(security.GetSymbol().GetCurrency(), currency);
	const auto rawStopPrice = security.DescalePrice(stopPrice);
	PlacedOrder order = {};
	order.id = m_client->PlaceBuyOrderWithMarketPrice(
		security,
		qty,
		rawStopPrice,
		params);
	order.security = &security;
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::SendBuyImmediatelyOrCancel(
		trdk::Security &security,
		const Currency &currency,
		const Qty &qty,
		const ScaledPrice &price,
		const OrderParams &params,
		const OrderStatusUpdateSlot &statusUpdateSlot) {
	AssertEq(security.GetSymbol().GetCurrency(), currency);
	const double rawPrice = security.DescalePrice(price);
	const PlacedOrder order = {
		m_client->PlaceBuyIocOrder(security, qty, rawPrice, params),
		&security,
		statusUpdateSlot};
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::SendBuyAtMarketPriceImmediatelyOrCancel(
		trdk::Security &,
		const trdk::Lib::Currency &,
		const trdk::Qty &,
		const trdk::OrderParams &,
		const OrderStatusUpdateSlot &) {
	throw MethodDoesNotImplementedError("Method is not implemented");
}

void ib::TradeSystem::RegOrder(const PlacedOrder &order) {
	const OrdersWriteLock lock(m_ordersMutex);
	Assert(
		m_placedOrders.get<ByOrder>().find(order.id)
		== m_placedOrders.get<ByOrder>().end());
	m_placedOrders.insert(order);
}
