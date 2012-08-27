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
	: public Trader::TradeSystem,
	public LiveMarketDataSource {

public:

	explicit InteractiveBrokersTradeSystem();
	virtual ~InteractiveBrokersTradeSystem();

public:

	virtual void Connect(const Settings &);

	virtual bool IsCompleted(const Security &) const;

public:

	virtual OrderId SellAtMarketPrice(
			const Security &,
			OrderQty,
			const OrderStatusUpdateSlot &);
	virtual OrderId Sell(
			const Security &,
			OrderQty,
			OrderPrice,
			const OrderStatusUpdateSlot &);
	virtual OrderId SellAtMarketPriceWithStopPrice(
			const Security &,
			OrderQty,
			OrderPrice stopPrice,
			const OrderStatusUpdateSlot &);
	virtual OrderId SellOrCancel(
			const Security &,
			OrderQty,
			OrderPrice,
			const OrderStatusUpdateSlot &);

	virtual OrderId BuyAtMarketPrice(
			const Security &,
			OrderQty,
			const OrderStatusUpdateSlot &);
	virtual OrderId Buy(
			const Security &,
			OrderQty,
			OrderPrice,
			const OrderStatusUpdateSlot &);
	virtual OrderId BuyAtMarketPriceWithStopPrice(
			const Security &,
			OrderQty,
			OrderPrice stopPrice,
			const OrderStatusUpdateSlot &);
	virtual OrderId BuyOrCancel(
			const Security &,
			OrderQty,
			OrderPrice,
			const OrderStatusUpdateSlot &);

	virtual void CancelOrder(OrderId);
	virtual void CancelAllOrders(const Security &);

public:

	virtual void SubscribeToMarketDataLevel1(boost::shared_ptr<Security>) const;
	virtual void SubscribeToMarketDataLevel2(boost::shared_ptr<Security>) const;

private:

	class Implementation;
	Implementation *m_pimpl;

};
