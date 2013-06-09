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

	class TRDK_CORE_API Security : public trdk::Instrument {

	public:

		typedef trdk::Instrument Base;

		typedef trdk::TradeSystem::OrderStatusUpdateSlot
			OrderStatusUpdateSlot;

		typedef void (Level1UpdateSlotSignature)();
		//! Update one of more from following values:
		//! best bid, best ask, last trade.
		typedef boost::function<Level1UpdateSlotSignature> Level1UpdateSlot;
		typedef boost::signals2::connection Level1UpdateSlotConnection;

		typedef void (Level1TickSlotSignature)(
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &,
					bool flush);
		typedef boost::function<Level1TickSlotSignature> Level1TickSlot;
		typedef boost::signals2::connection Level1TickSlotConnection;

		typedef void (NewTradeSlotSignature)(
					const boost::posix_time::ptime &,
					trdk::ScaledPrice,
					trdk::Qty,
					trdk::OrderSide);
		typedef boost::function<NewTradeSlotSignature> NewTradeSlot;
		typedef boost::signals2::connection NewTradeSlotConnection;

	public:

		explicit Security(trdk::Context &, const trdk::Lib::Symbol &);
		~Security();

	public:

		//! Check security for valid market data and state.
		bool IsStarted() const;
		bool IsLevel1Started() const;

	public:

		const char * GetCurrency() const {
			return "USD";
		}

		unsigned int GetPriceScale() const throw();

		trdk::ScaledPrice ScalePrice(double price) const;
		double DescalePrice(trdk::ScaledPrice price) const;

	public:

		boost::posix_time::ptime GetLastMarketDataTime() const;

		trdk::OrderId SellAtMarketPrice(
					trdk::Qty qty,
					trdk::Qty displaySize,
					trdk::Position &);
		trdk::OrderId Sell(
					trdk::Qty qty,
					trdk::ScaledPrice,
					trdk::Qty displaySize,
					trdk::Position &);
		trdk::OrderId SellAtMarketPriceWithStopPrice(
					trdk::Qty qty,
					ScaledPrice stopPrice,
					trdk::Qty displaySize,
					trdk::Position &);
		trdk::OrderId SellOrCancel(
					trdk::Qty,
					trdk::ScaledPrice,
					trdk::Position &);

		trdk::OrderId BuyAtMarketPrice(
					trdk::Qty qty,
					trdk::Qty displaySize,
					trdk::Position &);
		trdk::OrderId Buy(
					trdk::Qty,
					trdk::ScaledPrice,
					trdk::Qty displaySize,
					trdk::Position &);
		trdk::OrderId BuyAtMarketPriceWithStopPrice(
					trdk::Qty qty,
					ScaledPrice stopPrice,
					trdk::Qty displaySize,
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

		Level1UpdateSlotConnection SubcribeToLevel1Updates(
					const Level1UpdateSlot &)
				const;
		Level1UpdateSlotConnection SubcribeToLevel1Ticks(
					const Level1TickSlot &)
				const;
		NewTradeSlotConnection SubcribeToTrades(const NewTradeSlot &) const;

	protected:

		bool IsLevel1Required() const;
		bool IsLevel1UpdatesRequired() const;
		bool IsLevel1TicksRequired() const;
		bool IsTradesRequired() const;

		//! Sets one Level I parameter.
		/** @return	Subscribers will be notified about Level I Update only
		  *			if parameter will bee changed.
		  */
		void SetLevel1(
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &);
		//! Sets two Level I parameters and one operation.
		/** More optimal than call "set one parameter" two times.
		  * @return	Subscribers will be notified about Level I Update only
		  *			if parameter will bee changed.
		  */
		void SetLevel1(
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &,
					const trdk::Level1TickValue &);
		//! Sets three Level I parameters and one operation.
		/** More optimal than call "set one parameter" three times.
		  * @return	Subscribers will be notified about Level I Update only
		  *			if parameter will bee changed.
		  */
		void SetLevel1(
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &,
					const trdk::Level1TickValue &,
					const trdk::Level1TickValue &);

		//! Adds one Level I parameter tick.
		/** @return	Subscribers will be notified about Level I Update only
		  *			if parameter will bee changed. Level I Tick event will
		  *			be generated in any case.
		  */
		void AddLevel1Tick(
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &);
		//! Adds two Level I parameter ticks.
		/** More optimal than call "add one tick" two times.
		  * @return	Subscribers will be notified about Level I Update only
		  *			if parameter will bee changed. Level I Tick event will
		  *			be generated in any case.
		  */
		void AddLevel1Tick(
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &,
					const trdk::Level1TickValue &);
		//! Adds three Level I parameter ticks.
		/** More optimal than call "add one tick" three times.
		  * @return	Subscribers will be notified about Level I Update only
		  *			if parameter will bee changed. Level I Tick event will
		  *			be generated in any case.
		  */
		void AddLevel1Tick(
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &,
					const trdk::Level1TickValue &,
					const trdk::Level1TickValue &);

		void AddTrade(
				const boost::posix_time::ptime &,
				trdk::OrderSide,
				trdk::ScaledPrice,
				trdk::Qty,
				bool useAsLastTrade,
				bool useForTradedVolume);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
