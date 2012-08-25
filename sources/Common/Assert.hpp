/**************************************************************************
 *   Created: May 19, 2012 11:48:58 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "DisableBoostWarningsBegin.h"
#	include <boost/assert.hpp>
#include "DisableBoostWarningsEnd.h"

#undef assert
#undef Assert
#undef Verify
#undef AssertFail

#define Assert(expr) BOOST_ASSERT(expr)
#define Verify(expr) BOOST_VERIFY(expr)

#if defined(BOOST_DISABLE_ASSERTS)
#	if defined(BOOST_ENABLE_ASSERT_HANDLER)
		static_assert(false, "Failed to find assert-break method");
#	endif
	namespace Detatil {
		void ReportAssertFail(const char *, const char *, int) throw();
	}
#	define AssertFail(reason) (void)(::Detatil::ReportAssertFail(reason, __FILE__, __LINE__))
#else
#	define AssertFail(reason) (void)(::boost::assertion_failed(reason, BOOST_CURRENT_FUNCTION, __FILE__, __LINE__))
#endif

#define assert(expr) Assert(expr)

namespace Detatil {
	void AssertFailNoExceptionImpl(const char *, const char *, long);
}

#define AssertFailNoException() \
	(void)(::Detatil::AssertFailNoExceptionImpl(BOOST_CURRENT_FUNCTION, __FILE__, __LINE__))
