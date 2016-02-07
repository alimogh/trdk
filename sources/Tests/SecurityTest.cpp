/*******************************************************************************
 *   Created: 2016/02/07 02:13:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MockMarketDataSource.hpp"
#include "MockContext.hpp"
#include "Core/PriceBook.hpp"
#include "Core/Security.hpp"

namespace pt = boost::posix_time;

namespace trdk { namespace Tests {

	TEST(DISABLED_SecurityTest, PriceBookAdjusting) {

		MockContext context;
		MockMarketDataSource dataSource(0, context, "test");
		Security security(
			context,
			Lib::Symbol(
				Lib::Symbol::SECURITY_TYPE_FOR_FUTURE_OPTION,
				"EUR/USD"),
			dataSource);

	}

} }
