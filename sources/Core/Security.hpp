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
#include "Types.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API Security
		: public Trader::Instrument,
		public boost::enable_shared_from_this<Security> {

	public:

		typedef Trader::Instrument Base;

		typedef Trader::TradeSystem::OrderStatusUpdateSlot OrderStatusUpdateSlot;

		typedef void (Level1UpdateSlotSignature)();
		typedef boost::function<Level1UpdateSlotSignature> Level1UpdateSlot;
		typedef SignalConnection<
				Level1UpdateSlot,
				boost::signals2::connection>
			Level1UpdateSlotConnection;

		typedef void (NewTradeSlotSignature)(
					const boost::posix_time::ptime &,
					Trader::ScaledPrice,
					Trader::Qty,
					Trader::OrderSide);
		typedef boost::function<NewTradeSlotSignature> NewTradeSlot;
		typedef SignalConnection<
				NewTradeSlot,
				boost::signals2::connection>
			NewTradeSlotConnection;

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

		Trader::ScaledPrice ScalePrice(double price) const {
			return Util::Scale(price, GetPriceScale());
		}

		double DescalePrice(Trader::ScaledPrice price) const {
			return Util::Descale(price, GetPriceScale());
		}

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

		Trader::ScaledPrice GetAskPriceScaled(size_t pos) const;
		double GetAskPrice(size_t pos) const;
		Trader::Qty GetAskQty(size_t pos) const;

		Trader::ScaledPrice GetBidPriceScaled(size_t pos) const;
		double GetBidPrice(size_t pos) const;
		Trader::Qty GetBidQty(size_t pos) const;

		Trader::Qty GetTradedVolume() const;

	public:

		Level1UpdateSlotConnection SubcribeToLevel1(const Level1UpdateSlot &) const;
		NewTradeSlotConnection SubcribeToTrades(const NewTradeSlot &) const;

	protected:

		//! Updates the current data time.
		void SetLastMarketDataTime(const boost::posix_time::ptime &);

		//! Set last trade unscaled price and size.
		/**	@return true if values ​​differ from the current, false otherwise
		  */
		bool SetLast(double price, Trader::Qty size);
		//! Set last trade scaled price and size.
		/**	@return true if values ​​differ from the current, false otherwise
		  */
		bool SetLast(Trader::ScaledPrice, Trader::Qty);

		//! Set current unscaled ask price and ask size.
		/**	@return true if values ​​differ from the current, false otherwise
		  */
		bool SetAsk(double price, Trader::Qty size, size_t pos);
		//! Set current scaled ask price and ask size.
		/**	@return true if values ​​differ from the current, false otherwise
		  */
		bool SetAsk(Trader::ScaledPrice, Trader::Qty, size_t pos);

		//! Set current unscaled bid price and bid size.
		/**	@return true if values ​​differ from the current, false otherwise
		  */
		bool SetBid(double price, Trader::Qty size, size_t pos);
		//! Set current scaled bid price and bid size.
		/**	@return true if values ​​differ from the current, false otherwise
		  */
		bool SetBid(Trader::ScaledPrice, Trader::Qty, size_t pos);

		void SignalLevel1Update();
		void SignalNewTrade(
				const boost::posix_time::ptime &,
				Trader::OrderSide,
				Trader::ScaledPrice,
				Trader::Qty);

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
