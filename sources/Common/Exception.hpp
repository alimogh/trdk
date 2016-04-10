/**************************************************************************
 *   Created: May 19, 2012 2:42:37 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include <exception>
#include <iosfwd>

namespace trdk { namespace Lib {

	//////////////////////////////////////////////////////////////////////////

	class Exception : public std::exception {

	public:

		explicit Exception(const char *what) throw();
		Exception(const Exception &) throw();
		virtual ~Exception() throw();

		Exception & operator =(const Exception &) throw();

		friend std::ostream & operator <<(
				std::ostream &,
				const trdk::Lib::Exception &);

	public:

		virtual const char * what() const throw();

	private:

		const char *m_what;
		bool m_doFree;

	};

	//////////////////////////////////////////////////////////////////////////

	class LogicError : public trdk::Lib::Exception {
	public:
		explicit LogicError(const char *what) throw();
	};

	//////////////////////////////////////////////////////////////////////////

	class SystemException : public trdk::Lib::Exception {
	public:
		explicit SystemException(const char *what) throw();
	};

	//////////////////////////////////////////////////////////////////////////

	class MethodDoesNotImplementedError : public trdk::Lib::Exception {
	public:
		explicit MethodDoesNotImplementedError(const char *what) throw();
	};

	//////////////////////////////////////////////////////////////////////////

	class ModuleError : public trdk::Lib::Exception {
	public:
		explicit ModuleError(const char *what) throw();
	};

	//////////////////////////////////////////////////////////////////////////

	class RiskControlException : public trdk::Lib::Exception {
	public:
		explicit RiskControlException(const char *what) throw();
	};

	//////////////////////////////////////////////////////////////////////////

} }
