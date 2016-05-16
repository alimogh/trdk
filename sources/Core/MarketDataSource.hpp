/**************************************************************************
 *   Created: 2012/07/15 13:20:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Context.hpp"
#include "Interactor.hpp"
#include "Fwd.hpp"
#include "Api.h"

namespace trdk {

	////////////////////////////////////////////////////////////////////////////////

	//!	Market data source factory.
	/** Result can't be nullptr.
	  */
	typedef boost::shared_ptr<trdk::MarketDataSource> (MarketDataSourceFactory)(
			size_t index,
			trdk::Context &,
			const std::string &tag,
			const trdk::Lib::IniSectionRef &);

	////////////////////////////////////////////////////////////////////////////////

	class TRDK_CORE_API MarketDataSource : virtual public trdk::Interactor {

	public:

		typedef trdk::Interactor Base;

		typedef trdk::ModuleEventsLog Log;
		typedef trdk::ModuleTradingLog TradingLog;

	public:

		class TRDK_CORE_API Error : public Base::Error {
		public:
			explicit Error(const char *what) throw();
		};

	public:

		explicit MarketDataSource(
				size_t index,
				trdk::Context &,
				const std::string &tag);
		virtual ~MarketDataSource();

		bool operator ==(const MarketDataSource &rhs) const {
			return this == &rhs;
		}
		bool operator !=(const MarketDataSource &rhs) const {
			return !operator ==(rhs);
		}

		TRDK_CORE_API friend std::ostream & operator <<(
				std::ostream &,
				const trdk::MarketDataSource &);

	public:

		size_t GetIndex() const;

		trdk::Context & GetContext();
		const trdk::Context & GetContext() const;

		trdk::MarketDataSource::Log & GetLog() const throw();
		trdk::MarketDataSource::TradingLog & GetTradingLog() const throw();

		//! Identifies Market Data Source object by verbose name. 
		/** Market Data Source Tag unique, but can be empty for one of objects.
		  */
		const std::string & GetTag() const;
		const std::string & GetStringId() const throw();

	public:

		//! Makes connection with Market Data Source.
		virtual void Connect(const trdk::Lib::IniSectionRef &) = 0;

		virtual void SubscribeToSecurities() = 0;

	public:

		//! Returns security, creates new object if it doesn't exist yet.
		/** @sa trdk::MarketDataSource::FindSecurity
		  * @sa trdk::MarketDataSource::GetActiveSecurityCount
		  */
		trdk::Security & GetSecurity(const trdk::Lib::Symbol &);

		//! Finds security.
		/** @sa trdk::MarketDataSource::GetSecurity
		  * @sa trdk::MarketDataSource::GetActiveSecurityCount
		  * @return nullptr if security object doesn't exist.
		  */
		trdk::Security * FindSecurity(const trdk::Lib::Symbol &);
		//! Finds security.
		/** @sa trdk::MarketDataSource::GetSecurity
		  * @sa trdk::MarketDataSource::GetActiveSecurityCount
		  * @return nullptr if security object doesn't exist.
		  */
		const trdk::Security * FindSecurity(const trdk::Lib::Symbol &) const;

		size_t GetActiveSecurityCount() const;

		void ForEachSecurity(
				const boost::function<bool (const trdk::Security &)> &)
				const;

	public:

		//! CUSTOMIZED MERHOD for GadM. Switches security to new contact.
		virtual void SwitchToNewContract(trdk::Security &) = 0;

	protected:

		trdk::Security & CreateSecurity(const trdk::Lib::Symbol &);

		//! Creates security object.
		/** Each object, that implements CreateNewSecurityObject should wait
		  * for log flushing before destroying objects.
		  */
		virtual trdk::Security & CreateNewSecurityObject(
				const trdk::Lib::Symbol &)
			= 0;

	private:

		class Implementation;
		Implementation *const m_pimpl;

	};

	////////////////////////////////////////////////////////////////////////////////

}
