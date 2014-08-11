/**************************************************************************
 *   Created: 2014/08/09 13:08:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/TradeSystem.hpp"
#include "Core/MarketDataSource.hpp"

namespace trdk { namespace Interaction { namespace Onyx {

	//! Main OnixS FIX Engine wrapper.
	class OnixFixEngine
			: public trdk::TradeSystem,
			public trdk::MarketDataSource,
			public OnixS::FIX::ISessionListener {

	public:

		using Interactor::Error;

	public:

		explicit OnixFixEngine(
					const Lib::IniSectionRef &,
					Context::Log &);
		virtual ~OnixFixEngine();

	public:

		Context::Log & GetLog() const {
			return m_log;
		}

		const OnixS::FIX::ProtocolVersion::Enum & GetFixVersion() const {
			return m_fixVersion;
		}

	public:

		virtual void Connect(const trdk::Lib::IniSectionRef &);

		virtual void SubscribeToSecurities();

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
					OnixS::FIX::Session *) {
			//...//
		}
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

	protected:

		virtual boost::shared_ptr<trdk::Security> CreateSecurity(
					trdk::Context &,
					const trdk::Lib::Symbol &)
				const;

	protected:

		//! Implements trade system custom connect.
		virtual void ConnectSession(
					const trdk::Lib::IniSectionRef &,
					OnixS::FIX::Session &,
					const std::string &host,
					int port,
					const std::string &prefix);

	private:

		OnixS::FIX::Session * CreateSession(
					const trdk::Lib::IniSectionRef &,
					const std::string &prefix);

		void SubscribeToMarketData(OnixS::FIX::Session &);

	private:

		Context::Log &m_log;

		const OnixS::FIX::ProtocolVersion::Enum m_fixVersion;

		std::vector<boost::shared_ptr<Security>> m_securities;

		boost::scoped_ptr<OnixS::FIX::Session> m_tradeSession;
		boost::scoped_ptr<OnixS::FIX::Session> m_streamSession;

	
	};

} } }
