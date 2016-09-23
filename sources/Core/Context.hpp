/**************************************************************************
 *   Created: 2013/01/31 01:04:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "EventsLog.hpp"
#include "Fwd.hpp"
#include "Api.h"

namespace trdk {

	//////////////////////////////////////////////////////////////////////////

	class TRDK_CORE_API Context : private boost::noncopyable {

	public:

		class TRDK_CORE_API Exception : public trdk::Lib::Exception {
		public:
			explicit Exception(const char *what) throw();
		};
		class TRDK_CORE_API UnknownSecurity : public trdk::Context::Exception {
		public:
			explicit UnknownSecurity() throw();
		};

		typedef trdk::EventsLog Log;
		typedef trdk::TradingLog TradingLog;

		class TRDK_CORE_API Params;

		class TRDK_CORE_API DispatchingLock : private boost::noncopyable {
		public:
			virtual ~DispatchingLock() = 0;
		};

		typedef void (CurrentTimeChangeSlotSignature)(
					const boost::posix_time::ptime &newTime);
		typedef boost::function<CurrentTimeChangeSlotSignature>
			CurrentTimeChangeSlot;
		typedef boost::signals2::connection CurrentTimeChangeSlotConnection;

		enum State {
			STATE_ENGINE_STARTED,
			STATE_DISPATCHER_TASK_STOPPED_GRACEFULLY,
			STATE_DISPATCHER_TASK_STOPPED_ERROR,
			STATE_STRATEGY_BLOCKED,
			numberOfStates,
		};
		typedef void (StateUpdateSlotSignature)(
				const State &newState,
				const std::string *message /*= nullptr*/);
		typedef boost::function<StateUpdateSlotSignature> StateUpdateSlot;
		typedef boost::signals2::connection StateUpdateConnection;

	public:

		explicit Context(
				trdk::Context::Log &,
				trdk::Context::TradingLog &,
				const trdk::Settings &,
				const boost::posix_time::ptime &startTime);
		virtual ~Context();

	public:

		trdk::Context::Log & GetLog() const throw();
		trdk::Context::TradingLog & GetTradingLog() const throw();

		//! Subscribes to state changes.
		StateUpdateConnection SubscribeToStateUpdates(
				const StateUpdateSlot &)
				const;
		//! Raises state update event.
		void RaiseStateUpdate(const State &) const;
		//! Raises state update event with message.
		void RaiseStateUpdate(const State &, const std::string &message) const;

		trdk::Lib::TimeMeasurement::Milestones StartStrategyTimeMeasurement()
				const;
		trdk::Lib::TimeMeasurement::Milestones StartTradingSystemTimeMeasurement()
				const;
		trdk::Lib::TimeMeasurement::Milestones StartDispatchingTimeMeasurement()
				const;

		//! Context setting with predefined key list and predefined behavior.
		const trdk::Settings & GetSettings() const;

		const boost::posix_time::ptime & GetStartTime() const;

		boost::posix_time::ptime GetCurrentTime() const;

		//! Sets current time.
		/** If current time set  one time - real time will be used more.
		  * @param newTime				New time, can be only greater or equal
		  *								then current.
		  * @param signalAboutUpdate	If true - signal about update will be
		  *								sent for all subscribers.
		  * @sa SubscribeToCurrentTimeChange
		  */
		void SetCurrentTime(
				const boost::posix_time::ptime &newTime,
				bool signalAboutUpdate);
		//! Subscribes to time change by SetCurrentTime.
		/** Signals before new time will be set for context.
		  * @sa SetCurrentTime
		  */
		CurrentTimeChangeSlotConnection SubscribeToCurrentTimeChange(
				const CurrentTimeChangeSlot &)
				const;

		//! Waits until each of dispatching queue will be empty (but not all
		//! at the same moment).
		virtual std::unique_ptr<DispatchingLock> SyncDispatching() const = 0;

		virtual RiskControl & GetRiskControl(const trdk::TradingMode &) = 0;
		virtual const RiskControl & GetRiskControl(
				const trdk::TradingMode &)
				const
			= 0;

		template<typename Method>
		void InvokeDropCopy(const Method &method) const {
			auto *const dropCopy = GetDropCopy();
			if (!dropCopy) {
				return;
			}
			method(*dropCopy);
		}

		//! User context parameters. No predefined key list. Any key can be
		//! changed.
		trdk::Context::Params & GetParams();
		//! User context parameters. No predefined key list.
		const trdk::Context::Params & GetParams() const;

		trdk::Security & GetSecurity(const trdk::Lib::Symbol &);
		const trdk::Security & GetSecurity(const trdk::Lib::Symbol &) const;

		virtual const trdk::Lib::ExpirationCalendar & GetExpirationCalendar()
				const
			= 0;

		//! Market Data Sources count.
		/** @sa GetMarketDataSource
		  */
		virtual size_t GetNumberOfMarketDataSources() const = 0;
		//! Returns Market Data Source by index.
		/** Throws an exception if index in unknown.
		  * @sa GetNumberOfMarketDataSources
		  * @throw trdk::Lib::Exception
		  */
		virtual const trdk::MarketDataSource & GetMarketDataSource(
				size_t index)
				const
			= 0;
		//! Returns Market Data Source by index.
		/** Throws an exception if index in unknown.
		  * @sa GetNumberOfMarketDataSources
		  * @throw trdk::Lib::Exception
		  */
		virtual trdk::MarketDataSource & GetMarketDataSource(size_t index) = 0;
		//! Applies the given predicate to the each market data source and
		//! stops if predicate returns false.
		virtual void ForEachMarketDataSource(
				const boost::function<bool (const trdk::MarketDataSource &)> &)
				const
			= 0;
		//! Applies the given predicate to the each market data source and
		//! stops if predicate returns false.
		virtual void ForEachMarketDataSource(
				const boost::function<bool (trdk::MarketDataSource &)> &)
			= 0;

		//! Trading Systems count.
		/** @sa GetTradingSystem
		  */
		virtual size_t GetNumberOfTradingSystems() const = 0;
		//! Returns Trading System by index.
		/** Throws an exception if index in unknown.
		  * @sa GetNumberOfTradingSystems
		  * @throw trdk::Lib::Exception
		  */
		virtual const trdk::TradingSystem & GetTradingSystem(
				size_t index,
				const trdk::TradingMode &)
				const
			= 0;
		//! Returns Trading System by index.
		/** Throws an exception if index in unknown.
		  * @sa GetNumberOfTradingSystems
		  * @throw trdk::Lib::Exception
		  */
		virtual trdk::TradingSystem & GetTradingSystem(
				size_t index,
				const trdk::TradingMode &)
			= 0;

	protected:

		//! Returns Drop Copy or nullptr.
		virtual DropCopy * GetDropCopy() const = 0;
	
	protected:

		void OnStarted();
		void OnBeforeStop();

	protected:

		virtual trdk::Security * FindSecurity(const trdk::Lib::Symbol &) = 0;
		virtual const trdk::Security * FindSecurity(
				const trdk::Lib::Symbol &)
				const
			= 0;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	class trdk::Context::Params : private boost::noncopyable {

	public:

		class TRDK_CORE_API Exception : public trdk::Context::Exception {
		public:
			Exception(const char *what) throw();
			~Exception();
		};

		class TRDK_CORE_API KeyDoesntExistError : public Exception {
		public:
			KeyDoesntExistError(const char *what) throw();
			~KeyDoesntExistError();
		};

		typedef uintmax_t Revision;
	
	public:
	
		Params(const trdk::Context &);
		~Params();

	public:

		//! Returns key value.
		/** Throws an exception if key doesn't exist.
		  * @sa trdk::Context::Parameters::Update
		  * @throw trdk::Context::Parameters::KeyDoesntExistError
		  */
		std::string operator [](const std::string &) const;

	public:
		
		//! Returns current object revision.
		/** Any field update changes revision number. Update rule isn't defined.
		  */
		Revision GetRevision() const;

		bool IsExist(const std::string &) const;

		//! Updates key. Creates new if key doesn't exist.
		void Update(const std::string &key, const std::string &value);
	
	private:
	
		class Implementation;
		Implementation *m_pimpl;
	
	};

	////////////////////////////////////////////////////////////////////////////////

}
