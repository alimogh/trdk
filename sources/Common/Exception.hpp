/**************************************************************************
 *   Created: May 19, 2012 2:42:37 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include <exception>
#include <iosfwd>

namespace Trader { namespace Lib {

	//////////////////////////////////////////////////////////////////////////

	class Exception : public std::exception {

	public:

		explicit Exception(const char *what) throw();
		Exception(const Exception &) throw();
		virtual ~Exception() throw();

		Exception & operator =(const Exception &) throw();

	public:

		virtual const char * what() const throw();

	private:

		const char *m_what;
		bool m_doFree;

	};

	//////////////////////////////////////////////////////////////////////////

	class MethodDoesNotImplementedError : public Trader::Lib::Exception {

	public:

		explicit MethodDoesNotImplementedError(const char *what) throw();

	};

	//////////////////////////////////////////////////////////////////////////

} }

namespace std {

	std::ostream & operator <<(
			std::ostream &,
			const Trader::Lib::Exception &);

}
