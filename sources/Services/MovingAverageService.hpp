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


	public:

		explicit MovingAverageService(
					Context &,
					const std::string &tag,
					const Lib::IniFileSectionRef &);
		virtual ~MovingAverageService();

	public:

		//! Number of values.
		size_t GetSize() const;
		bool IsEmpty() const;

		//! Returns value by index.
		/** First bar has index "zero".
		  * @throw trdk::Services::MovingAverageService::ValueDoesNotExistError
		  * @sa trdk::Services::MovingAverageService::GetValueByReversedIndex
		  */
		trdk::ScaledPrice GetValue(size_t index) const;

		//! Returns value by reversed index.
		/** Last bar has index "zero".
		  * @throw trdk::Services::MovingAverageService::ValueDoesNotExistError
		  * @sa trdk::Services::MovingAverageService::GetValue 
		  */
		trdk::ScaledPrice GetValueByReversedIndex(size_t index) const;

		//! Returns last value.
		/** Last bar has index "zero".
		  * @throw trdk::Services::MovingAverageService::ValueDoesNotExistError
		  * @sa trdk::Services::MovingAverageService::GetValueByReversedIndex 
		  */
		trdk::ScaledPrice GetLastValue() const;

	public:

		virtual bool OnServiceDataUpdate(const trdk::Service &);

	protected:

		virtual void UpdateAlogImplSettings(
					const trdk::Lib::IniFileSectionRef &);
	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} }
