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

#include "CurrenexFixSession.hpp"
#include "Core/TradeSystem.hpp"

namespace trdk { namespace Interaction { namespace Onyx {

	//! FIX trade connection with OnixS C++ FIX Engine.
	class CurrenexFixTradeSystem
			: public trdk::TradeSystem,
			public OnixS::FIX::ISessionListener {

	public:

		explicit CurrenexFixTradeSystem(
					const Lib::IniSectionRef &,
					Context::Log &);
		virtual ~CurrenexFixTradeSystem();

	public:

		virtual void Connect(const trdk::Lib::IniSectionRef &);

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

		Context::Log &m_log;
		CurrenexFixSession m_session;		

	};

} } }
