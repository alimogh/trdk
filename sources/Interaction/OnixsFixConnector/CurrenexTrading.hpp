/**************************************************************************
 *   Created: 2014/08/12 22:17:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "CurrenexSession.hpp"
#include "Core/TradeSystem.hpp"

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	//! FIX trade connection with OnixS C++ FIX Engine.
	class CurrenexTrading
			: public trdk::TradeSystem,
			public OnixS::FIX::ISessionListener {

	public:

		explicit CurrenexTrading(
					const std::string &tag,
					const Lib::IniSectionRef &,
					Context::Log &);
		virtual ~CurrenexTrading();

	public:

		virtual void Connect(const trdk::Lib::IniSectionRef &);

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
		virtual OrderId SellOrCancel(
					trdk::Security &,
					const trdk::Lib::Currency &,
					trdk::Qty,
					trdk::ScaledPrice,
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
		virtual OrderId BuyOrCancel(
					trdk::Security &,
					const trdk::Lib::Currency &,
					trdk::Qty,
					trdk::ScaledPrice,
					const trdk::OrderParams &,
					const OrderStatusUpdateSlot &);

		virtual void CancelOrder(OrderId);
		virtual void CancelAllOrders(trdk::Security &);

	public:

		virtual void onInboundApplicationMsg(
					OnixS::FIX::Message &,
					OnixS::FIX::Session *);
		virtual void onStateChange(
					OnixS::FIX::SessionState::Enum newState,
					OnixS::FIX::SessionState::Enum prevState,
					OnixS::FIX::Session *);
		virtual void onError (
					OnixS::FIX::ErrorReason::Enum,
					const std::string &description,
					OnixS::FIX::Session *session);
		virtual void onWarning (
					OnixS::FIX::WarningReason::Enum,
					const std::string &description,
					OnixS::FIX::Session *);

	private:

		//! Takes next free order ID.
		OrderId TakeOrderId() {
			return m_nextOrderId++;
		}

		//! Creates order FIX-message and sets common fields.
		OnixS::FIX::Message CreateOrderMessage(
				const OrderId &,
				const Security &,
				const trdk::Lib::Currency &,
				const Qty &);

	private:

		void OnOrderNew(const OnixS::FIX::Message &);
		void OnOrderRejected(const OnixS::FIX::Message &);
		void OnOrderFill(const OnixS::FIX::Message &);
		void OnOrderPartialFill(const OnixS::FIX::Message &);

	private:

		Context::Log &m_log;
		CurrenexFixSession m_session;

		boost::atomic<OrderId> m_nextOrderId;

	};

} } }
