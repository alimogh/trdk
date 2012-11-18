/**************************************************************************
 *   Created: 2012/11/14 22:07:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Service.hpp"

namespace Trader { namespace Services {

	//! Bars collection service.
	class BarService : public Trader::Service {

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

	private:

		enum Units {
			UNITS_SECONDS,
			UNITS_MINUTES,
			UNITS_HOURS,
			UNITS_DAYS,
			numberOfUnits
		};

		typedef std::map<size_t, Bar> Bars;

	public:

		explicit BarService(
					const std::string &tag,
					boost::shared_ptr<Trader::Security> &,
					const Trader::Lib::IniFileSectionRef &,
					const boost::shared_ptr<const Settings> &);
		
		virtual ~BarService() {
			//...//
		}

	public:

		virtual const std::string & GetName() const {
			static const std::string name = "BarService";
			return name;
		}

		virtual Revision GetCurrentRevision() const {
			return m_revision;
		}

		virtual void OnNewTrade(
					const boost::posix_time::ptime &,
					ScaledPrice,
					Qty,
					OrderSide);
			
	public:

		//! Each bar size.
 		boost::posix_time::time_duration GetBarSize() const {
			static_assert(numberOfOrderSides, "Units list changed.");
			switch (m_units) {
				case UNITS_SECONDS:
					return boost::posix_time::seconds(m_barSize);
				case UNITS_MINUTES:
					return boost::posix_time::minutes(m_barSize);
				case UNITS_HOURS:
					return boost::posix_time::hours(m_barSize);
				case UNITS_DAYS:
					return boost::posix_time::hours(m_barSize * 24);
				default:
					AssertFail("Unknown units type");
					throw Trader::Lib::Exception(
						"Unknown bar service units type");
			}
		}

	public:

		//! Number of bars.
		size_t GetSize() const {
			return m_size;
		}

		bool IsEmpty() const {
			return m_size == 0;
		}

// 		//! First bar time.
// 		/** @throw Trader::Services::BarService::BarDoesNotExistError
// 		*/
// 		const TimeUtc & GetFirstTime() const;
// 
// 		//! First bar date.
// 		/** @throw Trader::Services::BarService::BarDoesNotExistError
// 		*/
// 		const DateUtc & GetFirstDate() const;
// 
// 		//! Last bar time.
// 		/** @throw Trader::Services::BarService::BarDoesNotExistError
// 		*/
// 		const TimeUtc & GetLastTime() const;
// 
// 		//! Last bar date.
// 		/** @throw Trader::Services::BarService::BarDoesNotExistError
// 		*/
// 		const DateUtc & GetLastDate() const;
// 
// 	public:
// 
		//! Returns bar by index.
		/** First bar has index "zero".
		  * @throw Trader::Services::BarService::BarDoesNotExistError
		  * @sa Trader::Services::BarService::GetBarByReversedIndex
		  */
		const Bar & GetBar(size_t index) const {
			if (IsEmpty()) {
				throw BarDoesNotExistError("BarService is empty");
			} else if (index >= size_t(m_size)) {
				throw BarDoesNotExistError("Index is out of range of BarService");
			}
			const Lock lock(GetMutex());
			const Bars::const_iterator pos = m_bars.find(index);
			Assert(pos != m_bars.end());
			return pos->second;
		}

		//! Returns bar by reversed index.
		/** Last bar has index "zero".
		  * @throw Trader::Services::BarService::BarDoesNotExistError
		  * @sa Trader::Services::BarService::GetBarByIndex 
		  */
		const Bar & GetBarByReversedIndex(size_t index) const {
			if (IsEmpty()) {
				throw BarDoesNotExistError("BarService is empty");
			} else if (index >= size_t(m_size)) {
				throw BarDoesNotExistError("Index is out of range of BarService");
			}
			const Lock lock(GetMutex());
			const Bars::const_iterator pos = m_bars.find(m_size - index - 1);
			Assert(pos != m_bars.end());
			return pos->second;
		}

// 		//! Returns bar whose period falls in the requested time (current
// 		//! session).
// 		/** @throw Trader::Services::BarService::BarDoesNotExistError
// 		*/
// 		const Bar & GetBar(const TimeUtc &) const;
// 
		//! Returns bar whose period falls in the requested date and time.
		/** @throw Trader::Services::BarService::BarDoesNotExistError
		*/
// 		const Bar & GetBar(const TimeUtc &, const DateUtc &) const {
// 			if (IsEmpty()) {
// 				throw BarDoesNotExistError();
// 			}
// 		}

	protected:

		virtual void UpdateAlogImplSettings(
					const Trader::Lib::IniFileSectionRef &) {
			//...//
		}

	private:

		boost::posix_time::ptime GetBarEnd(
					const boost::posix_time::ptime &)
				const;

		void LogCurrentBar() const;

	private:

		volatile Revision m_revision;
		volatile long m_size;

		Units m_units;
		long m_barSize;

		Bars m_bars;
		Bar *m_currentBar;
		boost::posix_time::ptime m_currentBarEnd;

		std::unique_ptr<std::ofstream> m_log;
			

	};

} }
