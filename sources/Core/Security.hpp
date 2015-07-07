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

		typedef uint32_t InstanceId;

		typedef trdk::TradeSystem::OrderStatusUpdateSlot
			OrderStatusUpdateSlot;

		typedef void (Level1UpdateSlotSignature)(
					const trdk::Lib::TimeMeasurement::Milestones &);
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

		//! Security broker position info.
		/** Information from broker, not relevant to trdk::Position.
		  */
		typedef void (BrokerPositionUpdateSlotSignature)(
				trdk::Qty,
				bool isInitial);
		typedef boost::function<BrokerPositionUpdateSlotSignature>
			BrokerPositionUpdateSlot;
		typedef boost::signals2::connection BrokerPositionUpdateSlotConnection;

		struct TRDK_CORE_API Bar {
			
			enum Type {
				TRADES,
				BID,
				ASK,
				numberOfTypes
			};

		 	//! Bar start time.
			boost::posix_time::ptime time;
			//! Bar size (time).
			boost::posix_time::time_duration size;
			//! Data type.
			Type type;
			
			//! The bar opening price.
			boost::optional<trdk::ScaledPrice> openPrice;
			//! The high price during the time covered by the bar.
			boost::optional<trdk::ScaledPrice> highPrice;
			
			//! The low price during the time covered by the bar.
			boost::optional<trdk::ScaledPrice> lowPrice;
			//! The bar closing price.
			boost::optional<trdk::ScaledPrice> closePrice;
			
			//! The volume during the time covered by the bar.
			boost::optional<trdk::Qty> volume;
			
			//! When TRADES historical data is returned, represents the number
			//! of trades that occurred during the time period the bar covers.
			boost::optional<size_t> count;

			explicit Bar(
		 				const boost::posix_time::ptime &,
						const boost::posix_time::time_duration &,
						Type);

		};
		typedef void (NewBarSlotSignature)(const Bar &);
		typedef boost::function<NewBarSlotSignature> NewBarSlot;
		typedef boost::signals2::connection NewBarSlotConnection;

		////////////////////////////////////////////////////////////////////////////////

		class BookUpdateOperation;
		class BookSideUpdateOperation;

		class TRDK_CORE_API Book {
			friend class trdk::Security::BookUpdateOperation;
		public:
			class Level {
				friend class trdk::Security::BookSideUpdateOperation;
			public:
				Level()
					: m_price(.0),
					m_qty(0) {
				}
				explicit Level(
						const boost::posix_time::ptime &time,
						double price,
						const trdk::Qty &qty)
					: m_time(time),
					m_price(price),
					m_qty(qty) {
					//...//
				}
				void Swap(Level &rhs) throw() {
					std::swap(rhs.m_price, m_price);
					std::swap(rhs.m_qty, m_qty);
				}
			public:
				void operator +=(const Level &rhs) {
					m_qty += rhs.m_qty;
				}
				bool operator ==(const Level &rhs) const {
					return
						trdk::Lib::IsEqual(m_price,rhs.m_price)
							&& trdk::Lib::IsEqual(m_qty,rhs.m_qty);
				}
				bool operator !=(const Level &rhs) const {
					return !operator ==(rhs);
				}
				const boost::posix_time::ptime & GetTime() const {
					return m_time;
				}
				double GetPrice() const {
					return m_price;
				}
				const trdk::Qty & GetQty() const {
					return m_qty;
				}
			private:
				boost::posix_time::ptime m_time;
				double m_price;
				trdk::Qty m_qty;
			};
			class TRDK_CORE_API Side {
				friend class trdk::Security::BookUpdateOperation;
				friend class trdk::Security::BookSideUpdateOperation;
			public:
				Side();
				~Side();
			public:
				size_t GetLevelsCount() const;
				const trdk::Security::Book::Level & GetLevel(
						size_t levelIndex)
						const;
			private:
				class Implementation;
				Implementation *m_pimpl;
			};
		public:
			explicit Book(const boost::posix_time::ptime &, bool isAdjusted);
		public:
			const boost::posix_time::ptime & GetTime() const {
				return m_time;
			}
			bool IsAdjusted() const {
				return m_isAdjusted;
			}
			const trdk::Security::Book::Side & GetBids() const {
				return m_bids;
			}
			const trdk::Security::Book::Side & GetAsks() const {
				return m_offers;
			}
			const trdk::Security::Book::Side & GetOffers() const {
				return m_offers;
			}
		private:
			boost::posix_time::ptime m_time;
			bool m_isAdjusted;
			Side m_bids;
			Side m_offers;
		};

		typedef void (BookUpdateTickSlotSignature)(
				const boost::shared_ptr<const trdk::Security::Book> &,
				const trdk::Lib::TimeMeasurement::Milestones &);
		typedef boost::function<BookUpdateTickSlotSignature> BookUpdateTickSlot;
		typedef boost::signals2::connection BookUpdateTickSlotConnection;

		class TRDK_CORE_API BookSideUpdateOperation
			: private boost::noncopyable {
			friend class trdk::Security::BookUpdateOperation;
		public:
			explicit BookSideUpdateOperation(trdk::Security::Book::Side &);
		public:
			void Swap(std::vector<trdk::Security::Book::Level> &);
		private:
			trdk::Security::Book::Side &m_storage;
		};

		class TRDK_CORE_API BookUpdateOperation : private boost::noncopyable {
			friend class trdk::Security;
		private:
			explicit BookUpdateOperation(
					trdk::Security &security,
					const boost::posix_time::ptime &,
					bool isAdjusted);
		public:
			BookUpdateOperation(BookUpdateOperation &&);
			~BookUpdateOperation();
		public:
			BookSideUpdateOperation & GetBids();
			BookSideUpdateOperation & GetOffers();
			BookSideUpdateOperation & GetAsks();
		public:
			void Adjust();
			static bool Adjust(
					const trdk::Security &,
					std::vector<trdk::Security::Book::Level> &bids,
					std::vector<trdk::Security::Book::Level> &asks);
		public:
			void Commit(const trdk::Lib::TimeMeasurement::Milestones &);
		private:
			class Implementation;
			Implementation *m_pimpl;
		};

		////////////////////////////////////////////////////////////////////////////////

	public:

		explicit Security(
					trdk::Context &,
					const trdk::Lib::Symbol &,
					const trdk::MarketDataSource &);
		~Security();

	public:

		//! Returns Market Data Source object which provides market data for
		//! this security object.
		const trdk::MarketDataSource & GetSource() const;

		RiskControlSymbolContext & GetRiskControlContext(
				const trdk::TradingMode &);

	public:

		const trdk::Security::InstanceId & GetInstanceId() const;

		//! Check security for valid market data and state.
		bool IsStarted() const;
		bool IsLevel1Started() const;

		//! Sets requested data start time if it not later than existing.
		void SetRequestedDataStartTime(const boost::posix_time::ptime &);
		//! Returns requested data start time.
		/** @return	Requested time or boost::posix_time::not_a_date_time if not
		  *			set.
		  */
		const boost::posix_time::ptime & GetRequestedDataStartTime() const;

	public:

		uintmax_t GetPriceScale() const;
		uint8_t GetPricePrecision() const throw();

		trdk::ScaledPrice ScalePrice(double price) const;
		double DescalePrice(trdk::ScaledPrice price) const;
		double DescalePrice(double price) const;

	public:

		boost::posix_time::ptime GetLastMarketDataTime() const;

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

		//! Position size.
		/** Information from broker, not relevant to trdk::Position.
		  */
		trdk::Qty GetBrokerPosition() const;

		bool IsBookAdjusted() const;

	public:

		Level1UpdateSlotConnection SubscribeToLevel1Updates(
				const Level1UpdateSlot &)
				const;
		
		Level1UpdateSlotConnection SubscribeToLevel1Ticks(
				const Level1TickSlot &)
				const;
		
		NewTradeSlotConnection SubscribeToTrades(const NewTradeSlot &) const;
		
		BrokerPositionUpdateSlotConnection SubscribeToBrokerPositionUpdates(
				const BrokerPositionUpdateSlot &)
				const;
		
		NewBarSlotConnection SubscribeToBars(const NewBarSlot &) const;
		
		BookUpdateTickSlotConnection SubscribeToBookUpdateTicks(
				const BookUpdateTickSlot &)
				const;

	protected:

		bool IsLevel1Required() const;
		bool IsLevel1UpdatesRequired() const;
		bool IsLevel1TicksRequired() const;
		bool IsTradesRequired() const;
		bool IsBrokerPositionRequired() const;
		bool IsBarsRequired() const;

		//! Allows forcibly start Level 1 notification,
		//! even if not all Level 1 data received.
		void StartLevel1();
		//! Sets one Level I parameter.
		/** Subscribers will be notified about Level I Update only if parameter
		  * will bee changed.
		  */
		void SetLevel1(
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &,
					const trdk::Lib::TimeMeasurement::Milestones &);
		//! Sets two Level I parameters and one operation.
		/** More optimal than call "set one parameter" two times. Subscribers
		  * will be notified about Level I Update only if parameter will be
		  * changed.
		  */
		void SetLevel1(
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &,
					const trdk::Level1TickValue &,
					const trdk::Lib::TimeMeasurement::Milestones &);
		//! Sets three Level I parameters and one operation.
		/** More optimal than call "set one parameter" three times. Subscribers
		  * will be notified about Level I Update only if parameter will bee
		  * changed.
		  */
		void SetLevel1(
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &,
					const trdk::Level1TickValue &,
					const trdk::Level1TickValue &,
					const trdk::Lib::TimeMeasurement::Milestones &);
		//! Sets four Level I parameters and one operation.
		/** More optimal than call "set one parameter" four times. Subscribers
		  * will be notified about Level I Update only if parameter will bee
		  * changed.
		  */
		void SetLevel1(
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &,
					const trdk::Level1TickValue &,
					const trdk::Level1TickValue &,
					const trdk::Level1TickValue &,
					const trdk::Lib::TimeMeasurement::Milestones &);

		//! Adds one Level I parameter tick.
		/** Subscribers will be notified about Level I Update only if parameter
		  * will bee changed. Level I Tick event will be generated in any case.
		  */
		void AddLevel1Tick(
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &,
					const trdk::Lib::TimeMeasurement::Milestones &);
		//! Adds two Level I parameter ticks.
		/** More optimal than call "add one tick" two times. Subscribers will
		  * be notified about Level I Update only if parameter will be changed.
		  * Level I Tick event will be generated in any case.
		  */
		void AddLevel1Tick(
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &,
					const trdk::Level1TickValue &,
					const trdk::Lib::TimeMeasurement::Milestones &);
		//! Adds three Level I parameter ticks.
		/** More optimal than call "add one tick" three times. Subscribers will
		  * be notified about Level I Update only if parameter will be changed.
		  * Level I Tick event will be generated in any case.
		  */
		void AddLevel1Tick(
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &,
					const trdk::Level1TickValue &,
					const trdk::Level1TickValue &,
					const trdk::Lib::TimeMeasurement::Milestones &);

		void AddTrade(
					const boost::posix_time::ptime &,
					trdk::OrderSide,
					trdk::ScaledPrice,
					trdk::Qty,
					const trdk::Lib::TimeMeasurement::Milestones &,
					bool useAsLastTrade,
					bool useForTradedVolume);

		void AddBar(const Bar &);

		//! Sets security broker position info.
		/** Subscribers will be notified only if parameter will be changed.
		  * @param qty			Position size.
		  * @param isInitial	true if it initial data at start.
		  */
		void SetBrokerPosition(trdk::Qty qty, bool isInitial);

		trdk::Security::BookUpdateOperation StartBookUpdate(
				const boost::posix_time::ptime &,
				bool isRespected);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
