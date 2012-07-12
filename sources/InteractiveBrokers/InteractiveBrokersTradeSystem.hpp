/**************************************************************************
 *   Created: May 26, 2012 9:44:37 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#pragma once

#include "Core/TradeSystem.hpp"

class InteractiveBrokersTradeSystem : public TradeSystem {

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

	class Implementation;
	Implementation *m_pimpl;

};
