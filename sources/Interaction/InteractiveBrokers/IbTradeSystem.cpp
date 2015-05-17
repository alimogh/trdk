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


ib::TradeSystem::TradeSystem(
			const Lib::IniSectionRef &settings,
			Context::Log &log)
		: MarketDataSource("InteractiveBrokers"), //! @todo Setup market system name
		m_log(log),
		m_isTestSource(settings.ReadBoolKey("test_source", false)) {
	if (settings.ReadBoolKey("positions", false)) {
		m_positions.reset(new Positions);
	}
}

ib::TradeSystem::~TradeSystem() {
	//...//
}

void ib::TradeSystem::Connect(const IniSectionRef &settings) {

	if (m_client) {
		return;
	}

	Assert(m_securities.empty());

	std::unique_ptr<Account> account;
	std::unique_ptr<Client> client(
		new Client(
			*this,
			m_log,
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
					trdk::OrderId id,
					trdk::OrderId /*parentId*/,
					OrderStatus status,
					long filled,
					long remaining,
					double avgFillPrice,
					double lastFillPrice,
					const std::string &/*whyHeld*/,
					Client::OrderCallbackList &callBackList) {
			OrderStatusUpdateSlot callBack;
			{
				auto &index = m_placedOrders.get<ByOrder>();
				const OrdersWriteLock lock(m_ordersMutex);
				const auto pos = index.find(id);
				if (pos == index.end()) {
					return;
				}
				callBack = pos->callback;
				switch (status) {
					default:
						return;
					case ORDER_STATUS_FILLED:
						Assert(filled > 0);
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
				boost::bind(
					callBack,
					id,
					status,
					filled,
					remaining,
					avgFillPrice,
					lastFillPrice));
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
 		auto &security = **m_unsubscribedSecurities.begin();
		m_unsubscribedSecurities.pop_front();
		m_securities.insert(&security);
		m_client->SubscribeToMarketData(security);
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

boost::shared_ptr<trdk::Security> ib::TradeSystem::CreateSecurity(
			Context &context,
			const Symbol &symbol)
		const {
	boost::shared_ptr<ib::Security> result(
		new ib::Security(context, symbol, *this, m_isTestSource));
	m_unsubscribedSecurities.push_back(&*result);
	return result;
}

void ib::TradeSystem::CancelOrder(trdk::OrderId orderId) {
	m_client->CancelOrder(orderId);
}

void ib::TradeSystem::CancelAllOrders(trdk::Security &security) {
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

trdk::OrderId ib::TradeSystem::SellAtMarketPrice(
			trdk::Security &security,
			Qty qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, false);
	PlacedOrder order = {};
	order.id = m_client->PlaceSellOrder(security, qty, params);
	order.security = &security;
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}


trdk::OrderId ib::TradeSystem::Sell(
			trdk::Security &security,
			Qty qty,
			ScaledPrice price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, false);
	const auto rawPrice = security.DescalePrice(price);
	PlacedOrder order = {};
	order.id = m_client->PlaceSellOrder(security, qty, rawPrice, params);
	order.security = &security;
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::SellAtMarketPriceWithStopPrice(
			trdk::Security &security,
			Qty qty,
			ScaledPrice stopPrice,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, false);
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

trdk::OrderId ib::TradeSystem::SellOrCancel(
			trdk::Security &security,
			Qty qty,
			ScaledPrice price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, true);
	const double rawPrice = security.DescalePrice(price);
	const PlacedOrder order = {
		m_client->PlaceSellIocOrder(security, qty, rawPrice, params),
		&security,
		statusUpdateSlot
	};
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::BuyAtMarketPrice(
			trdk::Security &security,
			Qty qty,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, false);
	PlacedOrder order = {};
	order.id = m_client->PlaceBuyOrder(security, qty, params);
	order.security = &security;
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::Buy(
			trdk::Security &security,
			Qty qty,
			ScaledPrice price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, false);
	const auto rawPrice = security.DescalePrice(price);
	PlacedOrder order = {};
	order.id = m_client->PlaceBuyOrder(security, qty, rawPrice, params);
	order.security = &security;
	order.callback = statusUpdateSlot;
	RegOrder(order);
	return order.id;
}

trdk::OrderId ib::TradeSystem::BuyAtMarketPriceWithStopPrice(
			trdk::Security &security,
			Qty qty,
			ScaledPrice stopPrice,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, false);
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

trdk::OrderId ib::TradeSystem::BuyOrCancel(
			trdk::Security &security,
			Qty qty,
			ScaledPrice price,
			const OrderParams &params,
			const OrderStatusUpdateSlot &statusUpdateSlot) {
	Validate(qty, params, true);
	const double rawPrice = security.DescalePrice(price);
	const PlacedOrder order = {
		m_client->PlaceBuyIocOrder(security, qty, rawPrice, params),
		&security,
		statusUpdateSlot};
	RegOrder(order);
	return order.id;
}

void ib::TradeSystem::RegOrder(const PlacedOrder &order) {
	const OrdersWriteLock lock(m_ordersMutex);
	Assert(
		m_placedOrders.get<ByOrder>().find(order.id)
		== m_placedOrders.get<ByOrder>().end());
	m_placedOrders.insert(order);
}