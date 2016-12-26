/**************************************************************************
 *   Created: 2016/10/30 17:45:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "TransaqConnector.hpp"
#include "TransaqConnectorContext.hpp"
#include "Core/TradingSystem.hpp"

namespace trdk { namespace Interaction { namespace Transaq {
	
	class TradingSystem
		: public trdk::TradingSystem,
		protected TradingConnector {

	private:

		struct Order {
			const Security *security;
			OrderId id;
			OrderStatusUpdateSlot callback;
			Qty qty;
			mutable Qty remainingQty;
			mutable OrderStatus status;
			mutable Lib::TimeMeasurement::Milestones replyDelayMeasurement;
			TradingSystemOrderId tradingSystemOrderId;
		};

		struct ByOrderId {
			//...//
		};
		struct ByTradingSystemOrderId {
			//...//
		};

		typedef boost::multi_index_container<
				Order,
				boost::multi_index::indexed_by<
					boost::multi_index::hashed_unique<
						boost::multi_index::tag<ByOrderId>,
						boost::multi_index::member<
							Order,
							OrderId,
							&Order::id>>,
					boost::multi_index::hashed_unique<
						boost::multi_index::tag<ByTradingSystemOrderId>,
						boost::multi_index::member<
							Order,
							TradingSystemOrderId,
							&Order::tradingSystemOrderId>>>>
			Orders;

		typedef concurrency::critical_section OrdersMutex;
		typedef OrdersMutex::scoped_lock OrdersLock;

	public:

		explicit TradingSystem(
			const trdk::TradingMode &,
			size_t index,
			trdk::Context &,
			const std::string &instanceName,
			const Lib::IniSectionRef &);
		virtual ~TradingSystem();

	public:

		using trdk::TradingSystem::GetContext;
		using trdk::TradingSystem::GetLog;

	public:

		virtual bool IsConnected() const override;

	protected:

		virtual void CreateConnection(
				const trdk::Lib::IniSectionRef &)
				override;

		virtual OrderId SendSellAtMarketPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
				override;
		virtual OrderId SendSell(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &&)
				override;
		virtual OrderId SendSellAtMarketPriceWithStopPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
				override;
		virtual OrderId SendSellImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
				override;
		virtual OrderId SendSellAtMarketPriceImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
				override;

		virtual OrderId SendBuyAtMarketPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
				override;
		virtual OrderId SendBuy(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &&)
				override;
		virtual OrderId SendBuyAtMarketPriceWithStopPrice(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &stopPrice,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
				override;
		virtual OrderId SendBuyImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::ScaledPrice &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
				override;
		virtual OrderId SendBuyAtMarketPriceImmediatelyOrCancel(
				trdk::Security &,
				const trdk::Lib::Currency &,
				const trdk::Qty &,
				const trdk::OrderParams &,
				const OrderStatusUpdateSlot &)
				override;

		virtual void SendCancelOrder(const OrderId &) override;

	protected:

		virtual ConnectorContext & GetConnectorContext() override;

		virtual void OnOrderUpdate(
				const OrderId &,
				TradingSystemOrderId &&tradingSystemOrderId,
				OrderStatus &&,
				Qty &&remainingQty,
				std::string &&tradingSystemMessage,
				const Lib::TimeMeasurement::Milestones::TimePoint &)
				override;

		virtual void OnTrade(
				std::string &&id,
				TradingSystemOrderId &&,
				double price,
				Qty &&)
				override;

	private:

		void RegisterOrder(
				const Security &,
				const OrderId &,
				const Qty &,
				const OrderStatusUpdateSlot &&,
				const Lib::TimeMeasurement::Milestones &&);

	private:

		ConnectorContext m_connectorContext;

		OrdersMutex m_ordersMutex;
		Orders m_orders;

	};

} } }
