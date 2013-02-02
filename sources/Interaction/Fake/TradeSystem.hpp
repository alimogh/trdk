/**************************************************************************
 *   Created: 2012/07/23 23:13:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/TradeSystem.hpp"
#include "Core/Context.hpp"

namespace Trader { namespace Interaction { namespace Fake {

	class TradeSystem : public Trader::TradeSystem {

	public:

		TradeSystem(const Lib::IniFileSectionRef &, Context::Log &);
		virtual ~TradeSystem();

	public:

		virtual void Connect(
				const Lib::IniFile &,
				const std::string &section);
		virtual bool IsCompleted(const Security &) const;

	public:

		virtual OrderId SellAtMarketPrice(
				Security &,
				Qty,
				const OrderStatusUpdateSlot &);
		virtual OrderId Sell(
				Security &,
				Qty,
				ScaledPrice,
				const OrderStatusUpdateSlot &);
		virtual OrderId SellAtMarketPriceWithStopPrice(
				Security &,
				Qty,
				ScaledPrice stopPrice,
				const OrderStatusUpdateSlot &);
		virtual OrderId SellOrCancel(
				Security &,
				Qty,
				ScaledPrice,
				const OrderStatusUpdateSlot &);

		virtual OrderId BuyAtMarketPrice(
				Security &,
				Qty,
				const OrderStatusUpdateSlot &);
		virtual OrderId Buy(
				Security &,
				Qty,
				ScaledPrice,
				const OrderStatusUpdateSlot &);
		virtual OrderId BuyAtMarketPriceWithStopPrice(
				Security &,
				Qty,
				ScaledPrice stopPrice,
				const OrderStatusUpdateSlot &);
		virtual OrderId BuyOrCancel(
				Security &,
				Qty,
				ScaledPrice,
				const OrderStatusUpdateSlot &);

		virtual void CancelOrder(OrderId);
		virtual void CancelAllOrders(Security &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} } }
