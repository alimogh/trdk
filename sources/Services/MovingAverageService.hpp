/**************************************************************************
 *   Created: 2013/10/23 19:44:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "BarService.hpp"
#include "Core/Service.hpp"
#include "Api.h"

namespace trdk { namespace Services {

	class TRDK_SERVICES_API MovingAverageService : public trdk::Service {

	public:

		//! General service error.
		class Error : public trdk::Lib::Exception {
		public:
			explicit Error(const char *) throw();
		};

		//! Throws when client code requests value which does not exist.
		class ValueDoesNotExistError : public Error {
		public:
			explicit ValueDoesNotExistError(const char *) throw();
		};

		//! Service has not points history.
		class HasNotHistory : public Error {
		public:
			explicit HasNotHistory(const char *) throw();
		};

		//! Value data point.
 		struct Point {
			trdk::ScaledPrice value;
		};

	public:

		explicit MovingAverageService(
					Context &,
					const std::string &tag,
					const Lib::IniFileSectionRef &);
		virtual ~MovingAverageService();

	public:

		//! Returns last value point.
		/** Last bar has index "zero".
		  * @throw trdk::Services::MovingAverageService::ValueDoesNotExistError
		  */
		Point GetLastPoint() const;

	public:

		//! Number of points from history.
		size_t GetHistorySize() const;

		//! Returns value point from history by index.
		/** First bar has index "zero".
		  * @throw trdk::Services::MovingAverageService::ValueDoesNotExistError
		  * @throw trdk::Services::MovingAverageService::HasNotHistory
		  * @sa trdk::Services::MovingAverageService::GetValueByReversedIndex
		  */
		Point GetHistoryPoint(size_t index) const;

		//! Returns value point from history by reversed index.
		/** Last bar has index "zero".
		  * @throw trdk::Services::MovingAverageService::ValueDoesNotExistError
		  * @throw trdk::Services::MovingAverageService::HasNotHistory
		  * @sa trdk::Services::MovingAverageService::GetValue 
		  */
		Point GetHistoryPointByReversedIndex(size_t index) const;

	public:

		virtual bool OnServiceDataUpdate(const trdk::Service &);

		bool OnNewBar(const trdk::Services::BarService::Bar &);

	protected:

		virtual void UpdateAlogImplSettings(
					const trdk::Lib::IniFileSectionRef &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} }
