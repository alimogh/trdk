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

		typedef trdk::TradeSystem Base;

	public:

		TradeSystem(
				Context &context,
				const std::string &tag,
				const Lib::IniSectionRef &);
		virtual ~TradeSystem();

	public:

		virtual void Connect(const Lib::IniSectionRef &);

	public:

		virtual OrderId SellAtMarketPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				trdk::Qty qty,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId Sell(
				trdk::Security &,
				const trdk::Lib::Currency &,
				trdk::Qty qty,
				trdk::ScaledPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SellAtMarketPriceWithStopPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				trdk::Qty qty,
				trdk::ScaledPrice stopPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SellImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SellAtMarketPriceImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);

		virtual OrderId BuyAtMarketPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				trdk::Qty qty,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId Buy(
				trdk::Security &,
				const trdk::Lib::Currency &,
				trdk::Qty qty,
				trdk::ScaledPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId BuyAtMarketPriceWithStopPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				trdk::Qty qty,
				trdk::ScaledPrice stopPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId BuyImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId BuyAtMarketPriceImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);

		virtual void CancelOrder(OrderId);
		virtual void CancelAllOrders(trdk::Security &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} } }
