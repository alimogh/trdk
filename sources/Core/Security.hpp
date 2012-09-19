	/**************************************************************************
 *   Created: May 14, 2012 9:07:07 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Instrument.hpp"
#include "Common/SignalConnection.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API Security
		: public Trader::Instrument,
		public boost::enable_shared_from_this<Security> {

	public:

		typedef Trader::Instrument Base;

		typedef Trader::TradeSystem::OrderStatusUpdateSlot OrderStatusUpdateSlot;

		typedef boost::posix_time::ptime MarketDataTime;

		typedef Trader::TradeSystem::OrderId OrderId;
		typedef Trader::TradeSystem::OrderQty Qty;
		typedef Trader::TradeSystem::OrderScaledPrice ScaledPrice;

		typedef void (UpdateSlotSignature)();
		typedef boost::function<UpdateSlotSignature> UpdateSlot;
		typedef SignalConnection<
				UpdateSlot,
				boost::signals2::connection>
			UpdateSlotConnection;

	public:

		explicit Security(
					boost::shared_ptr<Trader::TradeSystem>,
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					boost::shared_ptr<const Trader::Settings> settings,
					bool logMarketData);
		explicit Security(
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					boost::shared_ptr<const Trader::Settings> settings,
					bool logMarketData);

		~Security();

	public:

		//! Check security for valid market data and state.
		operator bool() const;

	public:

		const char * GetCurrency() const {
			return "USD";
		}

		unsigned int GetPriceScale() const throw() {
			return 100;
		}

		ScaledPrice ScalePrice(double price) const {
			return Util::Scale(price, GetPriceScale());
		}

		double DescalePrice(ScaledPrice price) const {
			return Util::Descale(price, GetPriceScale());
		}

	public:

		boost::posix_time::ptime GetLastMarketDataTime() const;

		OrderId SellAtMarketPrice(Qty, Trader::Position &);
		OrderId Sell(Qty, ScaledPrice, Trader::Position &);
		OrderId SellAtMarketPriceWithStopPrice(
					Qty,
					ScaledPrice stopPrice,
					Trader::Position &);
		OrderId SellOrCancel(
					Qty,
					ScaledPrice,
					Trader::Position &);

		OrderId BuyAtMarketPrice(Qty, Trader::Position &);
		OrderId Buy(Qty, ScaledPrice, Trader::Position &);
		OrderId BuyAtMarketPriceWithStopPrice(
					Qty,
					ScaledPrice stopPrice,
					Trader::Position &);
		OrderId BuyOrCancel(
					Qty,
					ScaledPrice,
					Trader::Position &);

		void CancelOrder(OrderId);
		void CancelAllOrders();

	public:

		ScaledPrice GetLastPriceScaled() const;
		double GetLastPrice() const;
		Qty GetLastSize() const;

		ScaledPrice GetAskPriceScaled() const;
		double GetAskPrice() const;
		Qty GetAskSize() const;

		ScaledPrice GetBidPriceScaled() const;
		double GetBidPrice() const;
		Qty GetBidSize() const;

	public:

		bool IsHistoryData() const;

	public:

		void OnHistoryDataStart();
		void OnHistoryDataEnd();

	public:

		UpdateSlotConnection Subcribe(const UpdateSlot &) const;

	protected:

		//! Updates the current data time.
		void SetLastMarketDataTime(const boost::posix_time::ptime &);

		//! Set last trade unscaled price and size.
		/**	@return true if values ​​differ from the current, false otherwise
		  */
		bool SetLast(double price, Qty size);
		//! Set last trade scaled price and size.
		/**	@return true if values ​​differ from the current, false otherwise
		  */
		bool SetLast(ScaledPrice, Qty);

		//! Set current unscaled ask price and ask size.
		/**	@return true if values ​​differ from the current, false otherwise
		  */
		bool SetAsk(double price, Qty size);
		//! Set current scaled ask price and ask size.
		/**	@return true if values ​​differ from the current, false otherwise
		  */
		bool SetAsk(ScaledPrice, Qty);

		//! Set current unscaled bid price and bid size.
		/**	@return true if values ​​differ from the current, false otherwise
		  */
		bool SetBid(double price, Qty size);
		//! Set current scaled bid price and bid size.
		/**	@return true if values ​​differ from the current, false otherwise
		  */
		bool SetBid(ScaledPrice, Qty);

		void SignalUpdate();

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
