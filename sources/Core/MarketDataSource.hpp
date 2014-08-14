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
			const std::string &tag,
			const trdk::Lib::IniSectionRef &,
			Context::Log &);

	////////////////////////////////////////////////////////////////////////////////

	class TRDK_CORE_API MarketDataSource : virtual public trdk::Interactor {

	public:

		typedef trdk::Interactor Base;

	public:

		class TRDK_CORE_API Error : public Base::Error {
		public:
			explicit Error(const char *what) throw();
		};

	public:

		MarketDataSource(const std::string &tag);
		virtual ~MarketDataSource();

	public:

		//! Identifies Market Data Source object by verbose name. 
		/** Market Data Source Tag unique, but can be empty for one of objects.
		  */
		const std::string & GetTag() const;

	public:

		//! Makes connection with Market Data Source.
		virtual void Connect(const trdk::Lib::IniSectionRef &) = 0;

		virtual void SubscribeToSecurities() = 0;

	public:

		//! Returns security, creates new object if it doesn't exist yet.
		/** @sa trdk::MarketDataSource::FindSecurity
		  * @sa trdk::MarketDataSource::GetActiveSecurityCount
		  */
		trdk::Security & GetSecurity(
					trdk::Context &,
					const trdk::Lib::Symbol &);

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

	protected:

		virtual boost::shared_ptr<trdk::Security> CreateSecurity(
					trdk::Context &,
					const trdk::Lib::Symbol &)
				const
				= 0;

	private:

		class Implementation;
		Implementation *const m_pimpl;

	};

	////////////////////////////////////////////////////////////////////////////////

}
