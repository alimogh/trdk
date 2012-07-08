/**************************************************************************
 *   Created: May 26, 2012 9:44:37 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#pragma once

#include "Core/TradeSystem.hpp"

class InteractiveBrokersClient;

class InteractiveBrokersTradeSystem : public TradeSystem {

private:

	typedef boost::shared_mutex Mutex;
	typedef boost::unique_lock<Mutex> WriteLock;
	typedef boost::shared_lock<Mutex> ReadLock;

	typedef std::map<OrderId, std::pair<std::string, OrderStatusUpdateSlot>>
		OrderToSecurity;
	typedef std::multimap<std::string, OrderId> SecurityToOrder;

public:

	explicit InteractiveBrokersTradeSystem();
	virtual ~InteractiveBrokersTradeSystem();

public:

	virtual void Connect();

	virtual bool IsCompleted(const Security &) const;

public:

	virtual void Sell(
			const Security &,
			OrderQty,
			OrderStatusUpdateSlot = OrderStatusUpdateSlot());
	virtual void Sell(
			const Security &,
			OrderQty,
			OrderPrice,
			OrderStatusUpdateSlot = OrderStatusUpdateSlot());
	virtual void SellAtMarketPrice(
			const Security &,
			OrderQty,
			OrderPrice stopPrice,
			OrderStatusUpdateSlot = OrderStatusUpdateSlot());
	virtual void SellOrCancel(
			const Security &,
			OrderQty,
			OrderPrice,
			OrderStatusUpdateSlot = OrderStatusUpdateSlot());

	virtual void Buy(
			const Security &,
			OrderQty,
			OrderStatusUpdateSlot = OrderStatusUpdateSlot());
	virtual void Buy(
			const Security &,
			OrderQty,
			OrderPrice,
			OrderStatusUpdateSlot = OrderStatusUpdateSlot());
	virtual void BuyAtMarketPrice(
			const Security &,
			OrderQty,
			OrderPrice stopPrice,
			OrderStatusUpdateSlot = OrderStatusUpdateSlot());
	virtual void BuyOrCancel(
			const Security &,
			OrderQty,
			OrderPrice,
			OrderStatusUpdateSlot = OrderStatusUpdateSlot());

	virtual void CancelAllOrders(const Security &);

	virtual const char * GetStringStatus(OrderStatus) const;

private:

	InteractiveBrokersClient *m_client;

	SecurityToOrder m_securityToOrder;
	OrderToSecurity m_orderToSecurity;
	mutable Mutex m_mutex;

};
