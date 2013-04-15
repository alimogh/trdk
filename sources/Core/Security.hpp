/**************************************************************************
 *   Created: May 14, 2012 9:07:07 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Instrument.hpp"
#include "TradeSystem.hpp"
#include "Api.h"

namespace trdk {

	class TRADER_CORE_API Security : public trdk::Instrument {

	public:

		typedef trdk::Instrument Base;

		typedef trdk::TradeSystem::OrderStatusUpdateSlot
			OrderStatusUpdateSlot;

		typedef void (Level1UpdateSlotSignature)();
		//! Update one of more from following values:
		//! best bid, best ask, last trade.
		typedef boost::function<Level1UpdateSlotSignature> Level1UpdateSlot;
		typedef boost::signals2::connection Level1UpdateSlotConnection;

		typedef void (NewTradeSlotSignature)(
					const boost::posix_time::ptime &,
					trdk::ScaledPrice,
					trdk::Qty,
					trdk::OrderSide);
		typedef boost::function<NewTradeSlotSignature> NewTradeSlot;
		typedef boost::signals2::connection NewTradeSlotConnection;

	public:

		explicit Security(
					trdk::Context &,
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					bool logMarketData);
		~Security();

	public:

		//! Check security for valid market data and state.
		bool IsStarted() const;

	public:

		const char * GetCurrency() const {
			return "USD";
		}

		unsigned int GetPriceScale() const throw();

		trdk::ScaledPrice ScalePrice(double price) const;
		double DescalePrice(trdk::ScaledPrice price) const;

	public:

		boost::posix_time::ptime GetLastMarketDataTime() const;

		trdk::OrderId SellAtMarketPrice(trdk::Qty, trdk::Position &);
		trdk::OrderId Sell(
					trdk::Qty,
					trdk::ScaledPrice,
					trdk::Position &);
		trdk::OrderId SellAtMarketPriceWithStopPrice(
					trdk::Qty,
					ScaledPrice stopPrice,
					trdk::Position &);
		trdk::OrderId SellOrCancel(
					trdk::Qty,
					trdk::ScaledPrice,
					trdk::Position &);

		trdk::OrderId BuyAtMarketPrice(trdk::Qty, trdk::Position &);
		trdk::OrderId Buy(
					trdk::Qty,
					trdk::ScaledPrice,
					trdk::Position &);
		trdk::OrderId BuyAtMarketPriceWithStopPrice(
					trdk::Qty,
					ScaledPrice stopPrice,
					trdk::Position &);
		trdk::OrderId BuyOrCancel(
					trdk::Qty,
					trdk::ScaledPrice,
					trdk::Position &);

		void CancelOrder(trdk::OrderId);
		void CancelAllOrders();

	public:

		trdk::ScaledPrice GetLastPriceScaled() const;
		double GetLastPrice() const;
		trdk::Qty GetLastQty() const;

		trdk::ScaledPrice GetAskPriceScaled() const;
		double GetAskPrice() const;
		trdk::Qty GetAskQty() const;

		trdk::ScaledPrice GetBidPriceScaled() const;
		double GetBidPrice() const;
		trdk::Qty GetBidQty() const;

		trdk::Qty GetTradedVolume() const;

	public:

		Level1UpdateSlotConnection SubcribeToLevel1(
					const Level1UpdateSlot &)
				const;
		NewTradeSlotConnection SubcribeToTrades(const NewTradeSlot &) const;

	protected:

		//! Updates the current data time.
		void SetLastMarketDataTime(const boost::posix_time::ptime &);

		bool IsLevel1Required() const;
		bool SetBidAsk(
				trdk::ScaledPrice bestBidPrice,
				trdk::Qty bestBidQty,
				trdk::ScaledPrice bestAskPrice,
				trdk::Qty bestAskQty);
		bool SetBidAsk(
				double bidPrice,
				trdk::Qty bidQty,
				double askPrice,
				trdk::Qty askQty);
		bool SetBidAskLast(
				trdk::ScaledPrice bidPrice,
				trdk::Qty bidQty,
				trdk::ScaledPrice askPrice,
				trdk::Qty askQty,
				trdk::ScaledPrice lastTradePrice,
				trdk::Qty lastTradeQty);
		bool SetBidAskLast(
				double bidPrice,
				trdk::Qty bidQty,
				double askPrice,
				trdk::Qty askQty,
				double lastTradePrice,
				trdk::Qty lastTradeQty);

		bool IsTradesRequired() const;
		void AddTrade(
				const boost::posix_time::ptime &,
				trdk::OrderSide,
				trdk::ScaledPrice,
				trdk::Qty,
				bool useAsLastTrade);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}

namespace std {
	TRADER_CORE_API std::ostream & operator <<(
				std::ostream &,
				const trdk::Security &);
}
