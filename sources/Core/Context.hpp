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

	public:

		explicit Context(
					trdk::Context::Log &,
					trdk::Context::TradingLog &,
					const trdk::Settings &);
		virtual ~Context();

	public:

		trdk::Context::Log & GetLog() const throw();
		trdk::Context::TradingLog & GetTradingLog() const throw();

		trdk::Lib::TimeMeasurement::Milestones StartStrategyTimeMeasurement()
				const;
		trdk::Lib::TimeMeasurement::Milestones StartTradeSystemTimeMeasurement()
				const;
		trdk::Lib::TimeMeasurement::Milestones StartDispatchingTimeMeasurement()
				const;

		//! Context setting with predefined key list and predefined behavior.
		const trdk::Settings & GetSettings() const;

	public:

		//! Waits until each of dispatching queue will be empty (but not all
		//! at the same moment).
		virtual void SyncDispatching() = 0;

		//! User context parameters. No predefined key list. Any key can be
		//! changed.
		trdk::Context::Params & GetParams();
		//! User context parameters. No predefined key list.
		const trdk::Context::Params & GetParams() const;

		trdk::Security & GetSecurity(const trdk::Lib::Symbol &);
		const trdk::Security & GetSecurity(const trdk::Lib::Symbol &) const;

		//! Market Data Sources count.
		/** @sa GetMarketDataSource
		  */
		virtual size_t GetMarketDataSourcesCount() const = 0;
		//! Returns Market Data Source by index.
		/** Throws an exception if index in unknown.
		  * @sa GetMarketDataSourcesCount
		  * @throw trdk::Lib::Exception
		  */
		virtual const trdk::MarketDataSource & GetMarketDataSource(
						size_t index)
					const
					= 0;
		//! Returns Market Data Source by index.
		/** Throws an exception if index in unknown.
		  * @sa GetMarketDataSourcesCount
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

		//! Trade Systems count.
		/** @sa GetTradeSystem
		  */
		virtual size_t GetTradeSystemsCount() const = 0;
		//! Returns Trade System by index.
		/** Throws an exception if index in unknown.
		  * @sa GetTradeSystemsCount
		  * @throw trdk::Lib::Exception
		  */
		virtual const trdk::TradeSystem & GetTradeSystem(
						size_t index)
					const
					= 0;
		//! Returns Trade System by index.
		/** Throws an exception if index in unknown.
		  * @sa GetTradeSystemsCount
		  * @throw trdk::Lib::Exception
		  */
		virtual trdk::TradeSystem & GetTradeSystem(size_t index) = 0;

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
