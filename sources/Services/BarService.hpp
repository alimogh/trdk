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

	public:

 		//! Bar data.
 		class Bar {
// 			openprice
// 			closeprice
// 			ScaledPrice min;
// 			ScaledPrice max;
// 			Foo foo;
 		};

	public:

		explicit BarService(
					const std::string &tag,
					boost::shared_ptr<Trader::Security> &security,
					const Trader::Lib::IniFileSectionRef &)
				: Service(tag, security),
				m_revision(0) {
			//...//
		}
		
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

	public:

// 	public:
// 
// 		//! Each bar size, seconds.
// 		Seconds GetSizeInSeconds() const;
// 
// 		//! Each bar size, whole minues.
// 		Minutes GetSizeInMinutes() const;
// 
// 		//! Each bar size, whole hours.
// 		Hours GetSizeInHours() const;
// 
// 		//! Each bar size, whole dyes.
// 		Dayes GetSizeInDayes() const;
// 
// 	public:
// 
// 		//! Number of bars.
// 		Size GetSize() const;
// 
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
// 		//! Returns bar by position.
// 		/** First bar has position "zero".
// 		* @throw Trader::Services::BarService::BarDoesNotExistError
// 		*/
// 		const Bar & GetBar(Position) const;
// 
// 		//! Returns bar by reversed position.
// 		/** Last bar has position "zero".
// 		* @throw Trader::Services::BarService::BarDoesNotExistError
// 		*/
// 		const Bar & GetBarByReversedPostion(Position) const;
// 
// 		//! Returns bar whose period falls in the requested time (current
// 		//! session).
// 		/** @throw Trader::Services::BarService::BarDoesNotExistError
// 		*/
// 		const Bar & GetBar(const TimeUtc &) const;
// 
// 		//! Returns bar whose period falls in the requested date and time.
// 		/** @throw Trader::Services::BarService::BarDoesNotExistError
// 		*/
// 		const Bar & GetBar(const TimeUtc &, const DateUtc &) const;

	protected:

		virtual void UpdateAlogImplSettings(
					const Trader::Lib::IniFileSectionRef &) {
			//...//
		}

	private:

		volatile Revision m_revision;

	};

} }
