/**************************************************************************
 *   Created: 2012/08/28 01:33:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/TradeSystem.hpp"

namespace Trader {  namespace Interaction { namespace Lightspeed {

	class Gateway : public Trader::TradeSystem {

	public:

		explicit Gateway();
		virtual ~Gateway();

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

	};

} } }