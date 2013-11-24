/**************************************************************************
 *   Created: 2012/07/23 23:13:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/TradeSystem.hpp"
#include "Core/Context.hpp"

namespace trdk { namespace Interaction { namespace Fake {

	class TradeSystem : public trdk::TradeSystem {

	public:

		TradeSystem(const Lib::IniFileSectionRef &, Context::Log &);
		virtual ~TradeSystem();

	public:

		virtual void Connect(const Lib::IniFileSectionRef &);

	public:

		virtual double GetCashBalance() const;

	public:

		virtual OrderId SellAtMarketPrice(
				trdk::Security &,
				trdk::Qty qty,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId Sell(
				trdk::Security &,
				trdk::Qty qty,
				trdk::ScaledPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SellAtMarketPriceWithStopPrice(
				trdk::Security &,
				trdk::Qty qty,
				trdk::ScaledPrice stopPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SellOrCancel(
				trdk::Security &,
				trdk::Qty,
				trdk::ScaledPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);

		virtual OrderId BuyAtMarketPrice(
				trdk::Security &,
				trdk::Qty qty,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId Buy(
				trdk::Security &,
				trdk::Qty qty,
				trdk::ScaledPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId BuyAtMarketPriceWithStopPrice(
				trdk::Security &,
				trdk::Qty qty,
				trdk::ScaledPrice stopPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId BuyOrCancel(
				trdk::Security &,
				trdk::Qty,
				trdk::ScaledPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);

		virtual void CancelOrder(OrderId);
		virtual void CancelAllOrders(trdk::Security &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} } }
