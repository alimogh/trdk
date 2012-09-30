/**************************************************************************
 *   Created: 2012/09/30 12:10:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"

#if defined(BOOST_ENABLE_ASSERT_HANDLER)

	namespace boost {
		void assertion_failed(
					char const *expr,
					char const *function,
					char const *file,
					long line) {
			boost::format place("function %1%, file %2%, line %3%");
			place % function % file % line;
			Q_ASSERT_X(false, place.str().c_str(), expr );
		}
	}

#endif
