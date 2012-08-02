/**************************************************************************
 *   Created: May 26, 2012 9:44:37 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#pragma once

#include "Core/TradeSystem.hpp"
#include "Core/MarketDataSource.hpp"

class InteractiveBrokersTradeSystem
	: public TradeSystem,
	public LiveMarketDataSource {

public:

	explicit InteractiveBrokersTradeSystem();
	virtual ~InteractiveBrokersTradeSystem();

public:

	virtual void Connect(const Settings &);

	virtual bool IsCompleted(const Security &) const;

public:

	virtual void SellAtMarketPrice(
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

	virtual void BuyAtMarketPrice(
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

	virtual void CancelOrder(OrderId);
	virtual void CancelAllOrders(const Security &);

public:

	void SubscribeToMarketDataLevel1(boost::shared_ptr<Security>) const;
	virtual void SubscribeToMarketDataLevel2(boost::shared_ptr<Security>) const;

private:

	class Implementation;
	Implementation *m_pimpl;

};
