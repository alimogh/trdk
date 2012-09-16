/**************************************************************************
 *   Created: 2012/07/23 23:13:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/TradeSystem.hpp"

namespace Trader { namespace Interaction { namespace Fake {

	class TradeSystem : public Trader::TradeSystem {

	public:

		TradeSystem();
		virtual ~TradeSystem();

	public:

		virtual void Connect(const IniFile &, const std::string &section);
		virtual bool IsCompleted(const Security &) const;

	public:

		virtual OrderId SellAtMarketPrice(
				const Security &,
				OrderQty,
				const OrderStatusUpdateSlot &);
		virtual OrderId Sell(
				const Security &,
				OrderQty,
				OrderScaledPrice,
				const OrderStatusUpdateSlot &);
		virtual OrderId SellAtMarketPriceWithStopPrice(
				const Security &,
				OrderQty,
				OrderScaledPrice stopPrice,
				const OrderStatusUpdateSlot &);
		virtual OrderId SellOrCancel(
				const Security &,
				OrderQty,
				OrderScaledPrice,
				const OrderStatusUpdateSlot &);

		virtual OrderId BuyAtMarketPrice(
				const Security &,
				OrderQty,
				const OrderStatusUpdateSlot &);
		virtual OrderId Buy(
				const Security &,
				OrderQty,
				OrderScaledPrice,
				const OrderStatusUpdateSlot &);
		virtual OrderId BuyAtMarketPriceWithStopPrice(
				const Security &,
				OrderQty,
				OrderScaledPrice stopPrice,
				const OrderStatusUpdateSlot &);
		virtual OrderId BuyOrCancel(
				const Security &,
				OrderQty,
				OrderScaledPrice,
				const OrderStatusUpdateSlot &);

		virtual void CancelOrder(OrderId);
		virtual void CancelAllOrders(const Security &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} } }
