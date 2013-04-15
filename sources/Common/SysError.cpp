/**************************************************************************
 *   Created: 2008/10/23 10:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: TunnelEx
 *       URL: http://tunnelex.net
 * Copyright: Eugene V. Palchukovsky
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "SysError.hpp"
#include "DisableBoostWarningsBegin.h"
#	include <boost/config.hpp>
#	include <boost/shared_ptr.hpp>
#include "DisableBoostWarningsEnd.h"
#ifdef BOOST_WINDOWS
#	include <Windows.h>
#else
#	include "DisableBoostWarningsBegin.h"
#		include <boost/system/system_error.hpp>
#	include "DisableBoostWarningsEnd.h"
#endif
#include "Assert.hpp"

using namespace trdk::Lib;

SysError::SysError(int errorNo) throw()
		: m_errorNo(errorNo) {
	//...//
}

SysError::~SysError() throw() {
	//...//
}

bool SysError::IsError() const {
#	ifdef BOOST_WINDOWS
		return GetErrorNo() != NOERROR;
#	else
		return GetErrorNo() != 0;
#	endif
}

bool SysError::CheckError() const {
#	ifdef BOOST_WINDOWS
		LPVOID buffer;
		const auto formatResult = ::FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			m_errorNo,
			0,
			(LPWSTR)&buffer,
			0,
			NULL);
		boost::shared_ptr<VOID> bufferPtr(buffer, &::LocalFree);
		return formatResult != 0;
#	else
		return true;
#	endif
}

#ifdef BOOST_WINDOWS

	namespace {

		template<typename String>
		const typename String::value_type * GetUnknownErrorTest() {
			return "Unknown error";
		}
		template<>
		const std::wstring::value_type * GetUnknownErrorTest<std::wstring>() {
			return L"Unknown error";
		}

		template<typename Char>
		bool IsLineEnd(Char ch) {
			return ch == '\n' || ch == '\r';
		}
		template<>
		bool IsLineEnd<std::wstring::value_type>(std::wstring::value_type ch) {
			return ch == L'\n' || ch == L'\r';
		}
	
		template<typename String, typename SysGetter>
		String GetStringFromSys(int errorNo, SysGetter &sysGetter) {
			LPVOID buffer;
			auto size = sysGetter(
				FORMAT_MESSAGE_ALLOCATE_BUFFER
					| FORMAT_MESSAGE_FROM_SYSTEM
					| FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				errorNo,
				0,
				(String::value_type *)&buffer,
				0,
				NULL);
			boost::shared_ptr<VOID> bufferPtr(buffer, &::LocalFree);
			String::value_type *const pch = static_cast<String::value_type *>(buffer);
			for ( ; size > 0 && IsLineEnd(pch[size - 1]) ; --size);
			if (size > 0) {
				pch[size - 1] = 0;
				return String(pch);
			} else {
				return String(GetUnknownErrorTest<String>());
			}
		}

	}

#endif

std::wstring SysError::GetStringW() const {
#	ifdef BOOST_WINDOWS
		return GetStringFromSys<std::wstring>(m_errorNo, ::FormatMessageW);
#	else // BOOST_WINDOWS
		AssertFail("Method not implemented");
		return L"";
#	endif // BOOST_WINDOWS
}

std::string SysError::GetString() const {
#	ifdef BOOST_WINDOWS
		return GetStringFromSys<std::string>(m_errorNo, ::FormatMessageA);
#	else // BOOST_WINDOWS
		return boost::system::system_error(m_errorNo, boost::system::get_system_category()).what();
#	endif // BOOST_WINDOWS
}

int SysError::GetErrorNo() const {
	return m_errorNo;
}

std::ostream & std::operator <<(std::ostream &os, const SysError &error) {
	os << error.GetString() << " (code: " << error.GetErrorNo() << ")";
	return os;
}

std::wostream & std::operator <<(std::wostream &os, const SysError &error) {
	os << error.GetStringW() << " (code: " << error.GetErrorNo() << ")";
	return os;
}
