/**************************************************************************
 *   Created: May 14, 2012 9:07:07 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Instrument.hpp"
#include "TradeSystem.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API Security : public Trader::Instrument {

	public:

		typedef Trader::Instrument Base;

		typedef Trader::TradeSystem::OrderStatusUpdateSlot
			OrderStatusUpdateSlot;

		typedef void (Level1UpdateSlotSignature)();
		//! Update one of more from following values:
		//! best bid, best ask, last trade.
		typedef boost::function<Level1UpdateSlotSignature> Level1UpdateSlot;
		typedef boost::signals2::connection Level1UpdateSlotConnection;

		typedef void (NewTradeSlotSignature)(
					const boost::posix_time::ptime &,
					Trader::ScaledPrice,
					Trader::Qty,
					Trader::OrderSide);
		typedef boost::function<NewTradeSlotSignature> NewTradeSlot;
		typedef boost::signals2::connection NewTradeSlotConnection;

	public:

		explicit Security(
					Trader::Context &,
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

		Trader::ScaledPrice ScalePrice(double price) const;
		double DescalePrice(Trader::ScaledPrice price) const;

	public:

		boost::posix_time::ptime GetLastMarketDataTime() const;

		Trader::OrderId SellAtMarketPrice(Trader::Qty, Trader::Position &);
		Trader::OrderId Sell(
					Trader::Qty,
					Trader::ScaledPrice,
					Trader::Position &);
		Trader::OrderId SellAtMarketPriceWithStopPrice(
					Trader::Qty,
					ScaledPrice stopPrice,
					Trader::Position &);
		Trader::OrderId SellOrCancel(
					Trader::Qty,
					Trader::ScaledPrice,
					Trader::Position &);

		Trader::OrderId BuyAtMarketPrice(Trader::Qty, Trader::Position &);
		Trader::OrderId Buy(
					Trader::Qty,
					Trader::ScaledPrice,
					Trader::Position &);
		Trader::OrderId BuyAtMarketPriceWithStopPrice(
					Trader::Qty,
					ScaledPrice stopPrice,
					Trader::Position &);
		Trader::OrderId BuyOrCancel(
					Trader::Qty,
					Trader::ScaledPrice,
					Trader::Position &);

		void CancelOrder(Trader::OrderId);
		void CancelAllOrders();

	public:

		Trader::ScaledPrice GetLastPriceScaled() const;
		double GetLastPrice() const;
		Trader::Qty GetLastQty() const;

		Trader::ScaledPrice GetAskPriceScaled() const;
		double GetAskPrice() const;
		Trader::Qty GetAskQty() const;

		Trader::ScaledPrice GetBidPriceScaled() const;
		double GetBidPrice() const;
		Trader::Qty GetBidQty() const;

		Trader::Qty GetTradedVolume() const;

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
				Trader::ScaledPrice bestBidPrice,
				Trader::Qty bestBidQty,
				Trader::ScaledPrice bestAskPrice,
				Trader::Qty bestAskQty);
		bool SetBidAsk(
				double bidPrice,
				Trader::Qty bidQty,
				double askPrice,
				Trader::Qty askQty);
		bool SetBidAskLast(
				Trader::ScaledPrice bidPrice,
				Trader::Qty bidQty,
				Trader::ScaledPrice askPrice,
				Trader::Qty askQty,
				Trader::ScaledPrice lastTradePrice,
				Trader::Qty lastTradeQty);
		bool SetBidAskLast(
				double bidPrice,
				Trader::Qty bidQty,
				double askPrice,
				Trader::Qty askQty,
				double lastTradePrice,
				Trader::Qty lastTradeQty);

		bool IsTradesRequired() const;
		void AddTrade(
				const boost::posix_time::ptime &,
				Trader::OrderSide,
				Trader::ScaledPrice,
				Trader::Qty,
				bool useAsLastTrade);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}

namespace std {
	TRADER_CORE_API std::ostream & operator <<(
				std::ostream &,
				const Trader::Security &);
}
