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
#include "TradingSystem.hpp"
#include "Api.h"

namespace trdk {

	class TRDK_CORE_API Security : public trdk::Instrument {

	public:

		typedef trdk::Instrument Base;

		typedef uint32_t InstanceId;

		class TRDK_CORE_API Request {
		public:
			Request();
			operator bool() const;
			void Swap(Request &) throw();
			//! Returns true if one or more fields have earlier value.
			/** Does not cancel the reverse.
			  */
			bool IsEarlier(const Request &) const;
		public:
			void RequestNumberOfTicks(size_t);
			void RequestTime(const boost::posix_time::ptime &);
			void Merge(const Request &);
		public:
			size_t GetNumberOfTicks() const;
			const boost::posix_time::ptime & GetTime() const;
		private:
			boost::posix_time::ptime m_time;
			size_t m_numberOfTicks;
		};

		typedef trdk::TradingSystem::OrderStatusUpdateSlot
			OrderStatusUpdateSlot;

		typedef std::bitset<trdk::numberOfLevel1TickTypes> SupportedLevel1Types;

		typedef void(Level1UpdateSlotSignature)(
				const trdk::Lib::TimeMeasurement::Milestones &);
		//! Update one of more from following values:
		//! best bid, best ask, last trade.
		typedef boost::function<Level1UpdateSlotSignature> Level1UpdateSlot;
		typedef boost::signals2::connection Level1UpdateSlotConnection;

		typedef void(Level1TickSlotSignature)(
				const boost::posix_time::ptime &,
				const trdk::Level1TickValue &,
				bool flush);
		typedef boost::function<Level1TickSlotSignature> Level1TickSlot;
		typedef boost::signals2::connection Level1TickSlotConnection;

		typedef void(NewTradeSlotSignature)(
				const boost::posix_time::ptime &,
				const trdk::ScaledPrice &,
				const trdk::Qty &);
		typedef boost::function<NewTradeSlotSignature> NewTradeSlot;
		typedef boost::signals2::connection NewTradeSlotConnection;

		//! Security broker position info.
		/** Information from broker, not relevant to trdk::Position.
		  */
		typedef void(BrokerPositionUpdateSlotSignature)(
				const trdk::Qty &,
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

		typedef void (BookUpdateTickSlotSignature)(
				const trdk::PriceBook &,
				const trdk::Lib::TimeMeasurement::Milestones &);
		typedef boost::function<BookUpdateTickSlotSignature> BookUpdateTickSlot;
		typedef boost::signals2::connection BookUpdateTickSlotConnection;

		////////////////////////////////////////////////////////////////////////////////

		class TRDK_CORE_API Exception : public trdk::Lib::Exception {
		public:
			explicit Exception(const char *what);
		};

		class TRDK_CORE_API MarketDataValueDoesNotExist
			: public trdk::Security::Exception {
		public:
			explicit MarketDataValueDoesNotExist(const char *what);
		};

		////////////////////////////////////////////////////////////////////////////////

		//! Security service event code.
		/** Notifies about system event for the specified security. Synchronous. 
		  * 
		  * @sa SubscribeToServiceEvents
		  */
		enum ServiceEvent {
			//! History data is loaded, security is online now.
			/** @sa SERVICE_EVENT_OFFLINE
			  */
			SERVICE_EVENT_ONLINE,
			//! Security is offline.
			/** @sa SERVICE_EVENT_ONLINE
			  */
			SERVICE_EVENT_OFFLINE,
			//! The current trading session is opened.
			/** @sa SERVICE_EVENT_TRADING_SESSION_CLOSED
			  */
			SERVICE_EVENT_TRADING_SESSION_OPENED,
			//! The current trading session is closed.
			/** There are no any special system limitation to trade or to get
			  * information if the session is closed.
			  * @sa SERVICE_EVENT_TRADING_SESSION_OPENED
			  */
			SERVICE_EVENT_TRADING_SESSION_CLOSED,
			//! Number of events.
			numberOfServiceEvents
		};

		typedef void (ServiceEventSlotSignature)(
			const boost::posix_time::ptime &,
			const trdk::Security::ServiceEvent &);
		typedef boost::function<ServiceEventSlotSignature> ServiceEventSlot;
		typedef boost::signals2::connection ServiceEventSlotConnection;

		////////////////////////////////////////////////////////////////////////////////

		typedef void (ContractSwitchingSlotSignature)(
			const boost::posix_time::ptime &,
			const trdk::Security::Request &);
		typedef boost::function<ContractSwitchingSlotSignature>
			ContractSwitchingSlot;
		typedef boost::signals2::connection ContractSwitchingSlotConnection;

		////////////////////////////////////////////////////////////////////////////////

	public:

		explicit Security(
				trdk::Context &,
				const trdk::Lib::Symbol &,
				trdk::MarketDataSource &,
				const SupportedLevel1Types &);
		virtual ~Security();

	public:

		//! Returns Market Data Source object which provides market data for
		//! this security object.
		const trdk::MarketDataSource & GetSource() const;

		RiskControlSymbolContext & GetRiskControlContext(
				const trdk::TradingMode &);

	public:

		const trdk::Security::InstanceId & GetInstanceId() const;

		//! Returns true if security is on-line, trading session is opend and
		//! security has market data.
		/** @sa IsOnline
		  * @sa IsTradingSessionOpened
		  */
		bool IsActive() const;

		//! Returns true if security is online.
		/** @sa IsActive
		  *	@sa IsTradingSessionOpened
		  */
		virtual bool IsOnline() const;

		//! Returns true if trading session is active at this moment.
		/** @sa IsActive
		  * @sa IsOnline
		  */
		bool IsTradingSessionOpened() const;

		//! Sets requested data start time if it is not later than existing.
		void SetRequest(const trdk::Security::Request &);
		//! Returns requested data start.
		const trdk::Security::Request & GetRequest() const;

	public:

		size_t GetLotSize() const;

		uintmax_t GetPriceScale() const;
		uint8_t GetPricePrecision() const throw();

		trdk::ScaledPrice ScalePrice(double price) const;
		double DescalePrice(const trdk::ScaledPrice &price) const;
		double DescalePrice(double price) const;

	public:

		boost::posix_time::ptime GetLastMarketDataTime() const;
		size_t TakeNumberOfMarketDataUpdates() const;

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

		//! Returns next expiration time.
		/** Throws exception if expiration is not provided.
		  * @sa GetExpiration
		  */
		virtual const trdk::Lib::ContractExpiration & GetExpiration() const;

		bool HasExpiration() const;

	public:

		//! Subscribes to contract switching event.
		/** Notification should be synchronous. Notification generator will wait
		  * until the event will be handled by each subscriber.
		  */
		ContractSwitchingSlotConnection SubscribeToContractSwitching(
				const ContractSwitchingSlot &)
				const;

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

		//! Subscribes to security service event.
		/** Notification should be synchronous. Notification generator will wait
		  * until the event will be handled by each subscriber.
		  */
		ServiceEventSlotConnection SubscribeToServiceEvents(
				const ServiceEventSlot &)
				const;

	protected:

		void SetOnline(const boost::posix_time::ptime &, bool isOnline);
		void SetTradingSessionState(
				const boost::posix_time::ptime &,
				bool isOpened);
		void SwitchTradingSession(const boost::posix_time::ptime &);

		bool IsLevel1Required() const;
		bool IsLevel1UpdatesRequired() const;
		bool IsLevel1TicksRequired() const;
		bool IsTradesRequired() const;
		bool IsBrokerPositionRequired() const;
		bool IsBarsRequired() const;

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
		//! Sets many Level I parameters and one operation.
		/** More optimal than call "set one parameter" several times.
		  *	Subscribers will be notified about Level I Update only if parameter
		  *	will bee changed.
		  */
		void SetLevel1(
				const boost::posix_time::ptime &,
				const std::vector<trdk::Level1TickValue> &,
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
		  * Level I Tick event will be generated in any case. First tick will be
		  * added first in the notification queue, last - will be added last.
		  * All ticks can have one type or various.
		  */
		void AddLevel1Tick(
				const boost::posix_time::ptime &,
				const trdk::Level1TickValue &,
				const trdk::Level1TickValue &,
				const trdk::Lib::TimeMeasurement::Milestones &);
		//! Adds three Level I parameter ticks.
		/** More optimal than call "add one tick" three times. Subscribers will
		  * be notified about Level I Update only if parameter will be changed.
		  * Level I Tick event will be generated in any case. First tick will be
		  * added first in the notification queue, last - will be added last.
		  * All ticks can have one type or various.
		  */
		void AddLevel1Tick(
				const boost::posix_time::ptime &,
				const trdk::Level1TickValue &,
				const trdk::Level1TickValue &,
				const trdk::Level1TickValue &,
				const trdk::Lib::TimeMeasurement::Milestones &);
		//! Adds four Level I parameter ticks.
		/** More optimal than call "add one tick" three times. Subscribers will
		  * be notified about Level I Update only if parameter will be changed.
		  * Level I Tick event will be generated in any case. First tick will be
		  * added first in the notification queue, last - will be added last.
		  * All ticks can have one type or various.
		  */
		void AddLevel1Tick(
				const boost::posix_time::ptime &,
				const trdk::Level1TickValue &,
				const trdk::Level1TickValue &,
				const trdk::Level1TickValue &,
				const trdk::Level1TickValue &,
				const trdk::Lib::TimeMeasurement::Milestones &);
		//! Adds many Level I parameter ticks.
		/** More optimal than call "add one tick" several times. Subscribers
		 ** will be notified about Level I Update only if parameter will be
		 ** changed. Level I Tick event will be generated in any case. First
		 ** tick will be added first in the notification queue, last - will be
		 ** added last. All ticks can have one type or various.
		  */
		void AddLevel1Tick(
				const boost::posix_time::ptime &,
				const std::vector<trdk::Level1TickValue> &,
				const trdk::Lib::TimeMeasurement::Milestones &);

		void AddTrade(
				const boost::posix_time::ptime &,
				const trdk::ScaledPrice &,
				const trdk::Qty &,
				const trdk::Lib::TimeMeasurement::Milestones &,
				bool useAsLastTrade,
				bool useForTradedVolume);

		void AddBar(const Bar &);

		//! Sets security broker position info.
		/** Subscribers will be notified only if parameter will be changed.
		  * @param qty			Position size.
		  * @param isInitial	true if it initial data at start.
		  */
		void SetBrokerPosition(const trdk::Qty &, bool isInitial);

		//! Sets price book snapshot with current data, but new time.
		/** Not thread-safe, caller should provide thread synchronization.
		  * @param book	New price book. Object can be used as temporary buffer
		  *				and changed by this call, may have wrong data and state
		  *				after.
		  * @param timeMeasurement	Time measurement object.
		  */
		void SetBook(
				trdk::PriceBook &book,
				const trdk::Lib::TimeMeasurement::Milestones &timeMeasurement);

		//! Sets new expiration.
		/** All marked data for security will be reset (so if security just
		  * started). The request will be reset. The request can be set new at
		  * the event handling.
		  * 
		  * @sa GetExpiration
		  * 
		  * @param time[in] Event time.
		  * @param expiration[in] New contract expiration.
		  * 
		  * @return	true if security has new request. Security will not have
		  *			new request if no one subscriber request or if the new
		  *			contract is first contract for security (if security
		  *			did not have expiration before) and if new request after
		  *			switching "is not earlier" the existing.
		  */
		bool SetExpiration(
				const boost::posix_time::ptime &time,
				const trdk::Lib::ContractExpiration &expiration);

	private:

		class Implementation;
		std::unique_ptr<Implementation> m_pimpl;

	};

}
