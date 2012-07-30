/**************************************************************************
 *   Created: 2012/07/23 23:13:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/TradeSystem.hpp"

class FakeTradeSystem : public TradeSystem {

public:

	FakeTradeSystem();
	virtual ~FakeTradeSystem();

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

public:

	virtual void SubscribeToMarketDataLevel2(boost::shared_ptr<Security>) const;

private:

	class Implementation;
	Implementation *m_pimpl;

};
