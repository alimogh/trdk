/**************************************************************************
 *   Created: May 19, 2012 2:42:37 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include <exception>

namespace Trader { namespace Lib {

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

} }
