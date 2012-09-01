/**************************************************************************
 *   Created: May 19, 2012 11:48:58 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "DisableBoostWarningsBegin.h"
#	include <boost/assert.hpp>
#	if !defined(BOOST_DISABLE_ASSERTS)
#		include <boost/lexical_cast.hpp>
#		include <string>
#	endif
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
	namespace Detail {
		void ReportAssertFail(const char *, const char *, int) throw();
	}
#	define AssertFail(reason) \
		(void)(::Detail::ReportAssertFail(reason, __FILE__, __LINE__))
#	define AssertEq(epected, expr) ((void)0)
#	define AssertNe(epected, expr) ((void)0)
#	define AssertGt(epected, expr) ((void)0)
#	define AssertGe(epected, expr) ((void)0)
#	define AssertLt(epected, expr) ((void)0)
#	define AssertLe(epected, expr) ((void)0)
#else

#	define AssertFail(reason) \
		(void)(::boost::assertion_failed( \
			reason, \
			BOOST_CURRENT_FUNCTION, \
			__FILE__, \
			__LINE__))

	namespace Detail {
		void ReportAssertEqFail(
				const std::string &expected,
				const std::string &real,
				const char *,
				const char *,
				const char *,
				int);
		void ReportAssertNeFail(
				const std::string &expected,
				const char *,
				const char *,
				const char *,
				int);
		void ReportAssertGtGeLtLeFail(
				const std::string &val1,
				const std::string &val2,
				const char *compType,
				const char *compOp,
				const char *expr1,
				const char *expr2,
				const char *function,
				const char *file,
				int line);
	}

#	define AssertEq(epxected, expr) \
		(epxected == (expr) \
			? (void)0 \
			: (void)::Detail::ReportAssertEqFail( \
				boost::lexical_cast<std::string>(epxected), \
				boost::lexical_cast<std::string>(expr), \
				#expr, \
				BOOST_CURRENT_FUNCTION, \
				__FILE__, \
				__LINE__))
#	define AssertNe(epected, expr) \
		(epxected != (expr) \
			? (void)0 \
			: (void)::Detail::ReportAssertNeFail( \
				boost::lexical_cast<std::string>(epxected), \
				#expr, \
				BOOST_CURRENT_FUNCTION, \
				__FILE__, \
				__LINE__))
#	define AssertGt(expr1, expr2) \
		(epxected > (expr2) \
			? (void)0 \
			: (void)::Detail::ReportAssertGtGeLtLeFail( \
				boost::lexical_cast<std::string>(expr1), \
				boost::lexical_cast<std::string>(expr2), \
				"GREATER THAN", \
				"<=", \
				#expr1, \
				#expr2, \
				BOOST_CURRENT_FUNCTION, \
				__FILE__, \
				__LINE__))
#	define AssertGe(expr1, expr2) \
		(expr1 >= (expr2) \
			? (void)0 \
			: (void)::Detail::ReportAssertGtGeLtLeFail( \
				boost::lexical_cast<std::string>(expr1), \
				boost::lexical_cast<std::string>(expr2), \
				"GREATER THAN or EQUAL", \
				"<", \
				#expr1, \
				#expr2, \
				BOOST_CURRENT_FUNCTION, \
				__FILE__, \
				__LINE__))
#	define AssertLt(expr1, expr2) \
		(expr1 < (expr2) \
			? (void)0 \
			: (void)::Detail::ReportAssertGtGeLtLeFail( \
				boost::lexical_cast<std::string>(expr1), \
				boost::lexical_cast<std::string>(expr2), \
				"LESS THAN", \
				">=", \
				#expr1, \
				#expr2, \
				BOOST_CURRENT_FUNCTION, \
				__FILE__, \
				__LINE__))
#	define AssertLe(expr1, expr2) \
		(expr1 <= (expr2) \
			? (void)0 \
			: (void)::Detail::ReportAssertGtGeLtLeFail( \
				boost::lexical_cast<std::string>(expr1), \
				boost::lexical_cast<std::string>(expr2), \
				"LESS THAN or EQUAL", \
				">", \
				#expr1, \
				#expr2, \
				BOOST_CURRENT_FUNCTION, \
				__FILE__, \
				__LINE__))

#endif

#define assert(expr) Assert(expr)

namespace Detail {
	void AssertFailNoExceptionImpl(const char *, const char *, long);
}

#define AssertFailNoException() \
	(void)(::Detail::AssertFailNoExceptionImpl(BOOST_CURRENT_FUNCTION, __FILE__, __LINE__))
