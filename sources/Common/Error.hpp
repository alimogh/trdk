/**************************************************************************
 *   Created: 2008/10/23 10:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 **************************************************************************/

#pragma once

#include <string>
#include "DisableBoostWarningsBegin.h"
#	include <boost/config.hpp>
#include "DisableBoostWarningsEnd.h"

////////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_WINDOWS

#	include <errno.h>

	inline int GetLastError() throw() {
		return errno;
	}

#endif

////////////////////////////////////////////////////////////////////////////////

namespace Trader { namespace Lib {	 

	class Error {

	public:

		explicit Error(int errorNo) throw();
		~Error() throw();

	public:

		std::wstring GetStringW() const;
		std::string GetString() const;
		int GetErrorNo() const;

		bool IsError() const;

		//! Returns true if error could be resolved to string.
		bool CheckError() const;

	private:

		int m_errorNo;

	};

} }

namespace std {

	std::ostream & operator <<(std::ostream &, const Trader::Lib::Error &);
	std::wostream & operator <<(std::wostream &, const Trader::Lib::Error &);

}

////////////////////////////////////////////////////////////////////////////////
