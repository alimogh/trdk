/**************************************************************************
 *   Created: May 19, 2012 11:48:58 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "DisableBoostWarningsBegin.h"
#	include <boost/assert.hpp>
#	if !defined(BOOST_DISABLE_ASSERTS) && defined(BOOST_ENABLE_ASSERT_HANDLER)
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

#ifndef FILE_ASSERT_HPP_INCLUDED
	namespace trdk { namespace Debug { namespace Detail {
		void RegisterUnhandledException(
				const char *,
				const char *,
				long,
				bool);
	} } }
#endif

#if defined(BOOST_DISABLE_ASSERTS) || !defined(BOOST_ENABLE_ASSERT_HANDLER)
#	if defined(BOOST_ENABLE_ASSERT_HANDLER)
		static_assert(false, "Failed to find assert-break method");
#	endif
#	ifndef FILE_ASSERT_HPP_INCLUDED
		namespace trdk { namespace Debug { namespace Detail {
			void ReportAssertFail(const char *, const char *, int) throw();
		} } }
#	endif
#	define AssertFail(reason) \
		(void)(::trdk::Debug::Detail::ReportAssertFail( \
			reason, \
			__FILE__, \
			__LINE__))
#	define AssertEq(expr1, expr2) ((void)0)
#	define AssertNe(expr1, expr2) ((void)0)
#	define AssertGt(expr1, expr2) ((void)0)
#	define AssertGe(expr1, expr2) ((void)0)
#	define AssertLt(expr1, expr2) ((void)0)
#	define AssertLe(expr1, expr2) ((void)0)
#elif defined(BOOST_ENABLE_ASSERT_HANDLER)

#	define AssertFail(reason) \
		(void)(::boost::assertion_failed( \
			reason, \
			BOOST_CURRENT_FUNCTION, \
			__FILE__, \
			__LINE__))

#	ifndef FILE_ASSERT_HPP_INCLUDED
		namespace trdk { namespace Debug { namespace Detail {
		
			void ReportCompareAssertFail(
					const std::string &val1,
					const std::string &val2,
					const char *compType,
					const char *compOp,
					const char *expr1,
					const char *expr2,
					const char *function,
					const char *file,
					int line);

			template<typename Source>
			std::string CastToString(const Source &source) {
				try {
					return boost::lexical_cast<std::string>(source);
				} catch (...) {
					trdk::Debug::Detail::RegisterUnhandledException(
						BOOST_CURRENT_FUNCTION,
						__FILE__,
						__LINE__,
						true);
					return std::string("[VARIABLE VALUE RETRIEVE ERROR]");
				}
			}

		} } }
#	endif

#	define AssertEq(expr1, expr2) \
		((expr1) == (expr2) \
			? (void)0 \
			: (void)::trdk::Debug::Detail::ReportCompareAssertFail( \
				trdk::Debug::Detail::CastToString(expr1), \
				trdk::Debug::Detail::CastToString(expr2), \
				"EQUAL", \
				"!=", \
				#expr1, \
				#expr2, \
				BOOST_CURRENT_FUNCTION, \
				__FILE__, \
				__LINE__))
#	define AssertNe(expr1, expr2) \
		((expr1) != (expr2) \
			? (void)0 \
			: (void)::trdk::Debug::Detail::ReportCompareAssertFail( \
				trdk::Debug::Detail::CastToString(expr1), \
				trdk::Debug::Detail::CastToString(expr2), \
				"NOT EQUAL", \
				"==", \
				#expr1, \
				#expr2, \
				BOOST_CURRENT_FUNCTION, \
				__FILE__, \
				__LINE__))
#	define AssertGt(expr1, expr2) \
		((expr1) > (expr2) \
			? (void)0 \
			: (void)::trdk::Debug::Detail::ReportCompareAssertFail( \
				trdk::Debug::Detail::CastToString(expr1), \
				trdk::Debug::Detail::CastToString(expr2), \
				"GREATER THAN", \
				"<=", \
				#expr1, \
				#expr2, \
				BOOST_CURRENT_FUNCTION, \
				__FILE__, \
				__LINE__))
#	define AssertGe(expr1, expr2) \
		((expr1) >= (expr2) \
			? (void)0 \
			: (void)::trdk::Debug::Detail::ReportCompareAssertFail( \
				trdk::Debug::Detail::CastToString(expr1), \
				trdk::Debug::Detail::CastToString(expr2), \
				"GREATER THAN or EQUAL", \
				"<", \
				#expr1, \
				#expr2, \
				BOOST_CURRENT_FUNCTION, \
				__FILE__, \
				__LINE__))
#	define AssertLt(expr1, expr2) \
		((expr1) < (expr2) \
			? (void)0 \
			: (void)::trdk::Debug::Detail::ReportCompareAssertFail( \
				trdk::Debug::Detail::CastToString(expr1), \
				trdk::Debug::Detail::CastToString(expr2), \
				"LESS THAN", \
				">=", \
				#expr1, \
				#expr2, \
				BOOST_CURRENT_FUNCTION, \
				__FILE__, \
				__LINE__))
#	define AssertLe(expr1, expr2) \
		((expr1) <= (expr2) \
			? (void)0 \
			: (void)::trdk::Debug::Detail::ReportCompareAssertFail( \
				trdk::Debug::Detail::CastToString(expr1), \
				trdk::Debug::Detail::CastToString(expr2), \
				"LESS THAN or EQUAL", \
				">", \
				#expr1, \
				#expr2, \
				BOOST_CURRENT_FUNCTION, \
				__FILE__, \
				__LINE__))

#endif

#define assert(expr) Assert(expr)

#ifndef FILE_ASSERT_HPP_INCLUDED
	namespace trdk { namespace Debug { namespace Detail {
		void AssertFailNoExceptionImpl(const char *, const char *, long);
	} } }
#endif

#define AssertFailNoException() \
	(void)(::trdk::Debug::Detail::AssertFailNoExceptionImpl( \
		BOOST_CURRENT_FUNCTION, \
		__FILE__, \
		__LINE__))

#ifndef FILE_ASSERT_HPP_INCLUDED
#	define FILE_ASSERT_HPP_INCLUDED
#endif
