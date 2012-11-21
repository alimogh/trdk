/**************************************************************************
 *   Created: 2012/11/14 22:07:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Service.hpp"
#include "Api.h"

namespace Trader { namespace Services {

	//! Bars collection service.
	class TRADER_SERVICES_API BarService : public Trader::Service {

	public:

		//! General service error.
		class Error : public Trader::Lib::Exception {
		public:
			explicit Error(const char *what) throw()
					:  Exception(what) {
				//...//
			}
		};

		//! Throws when client code requests bar which does not exist.
		class BarDoesNotExistError : public Error {
		public:
			explicit BarDoesNotExistError(const char *what) throw()
					:  Error(what) {
				//...//
			}
		};

 		//! Bar data.
 		struct Bar {
			ScaledPrice openPrice;
			ScaledPrice closePrice;
			ScaledPrice high;
			ScaledPrice low;
			Qty volume;
 		};

	public:

		explicit BarService(
					const std::string &tag,
					boost::shared_ptr<Trader::Security> &,
					const Trader::Lib::IniFileSectionRef &,
					const boost::shared_ptr<const Settings> &);
		
		virtual ~BarService();

	public:

		virtual const std::string & GetName() const;

		virtual Revision GetCurrentRevision() const;

		virtual void OnNewTrade(
					const boost::posix_time::ptime &,
					ScaledPrice,
					Qty,
					OrderSide);
			
	public:

		//! Each bar size.
 		boost::posix_time::time_duration GetBarSize() const;

	public:

		//! Number of bars.
		size_t GetSize() const;

		bool IsEmpty() const;

	public:

		//! Returns bar by index.
		/** First bar has index "zero".
		  * @throw Trader::Services::BarService::BarDoesNotExistError
		  * @sa Trader::Services::BarService::GetBarByReversedIndex
		  */
		const Bar & GetBar(size_t index) const;

		//! Returns bar by reversed index.
		/** Last bar has index "zero".
		  * @throw Trader::Services::BarService::BarDoesNotExistError
		  * @sa Trader::Services::BarService::GetBarByIndex 
		  */
		const Bar & GetBarByReversedIndex(size_t index) const;

 		//! Returns bar whose period falls in the requested time.
 		/** @throw Trader::Services::BarService::BarDoesNotExistError
 		  */
// 		const Bar & GetBar(const TimeUtc &) const;

	protected:

		virtual void UpdateAlogImplSettings(
					const Trader::Lib::IniFileSectionRef &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} }
