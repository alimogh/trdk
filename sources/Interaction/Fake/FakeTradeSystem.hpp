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
				size_t index,
				Context &context,
				const std::string &tag,
				const Lib::IniSectionRef &);
		virtual ~TradeSystem();

	public:

		virtual bool IsConnected() const;

	protected:

		virtual OrderId SendSellAtMarketPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendSell(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendSellAtMarketPriceWithStopPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &stopPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendSellImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendSellAtMarketPriceImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);

		virtual OrderId SendBuyAtMarketPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendBuy(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendBuyAtMarketPriceWithStopPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &stopPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendBuyImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);
		virtual OrderId SendBuyAtMarketPriceImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &);

		virtual void SendCancelOrder(const OrderId &);
		virtual void SendCancelAllOrders(trdk::Security &);

	public:

		virtual void CreateConnection(const Lib::IniSectionRef &);

	public:

		virtual void OnSettingsUpdate(const Lib::IniSectionRef &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} } }
