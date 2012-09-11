/**************************************************************************
 *   Created: 2012/09/11 01:34:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "MarketData.hpp"

namespace Trader {  namespace Interaction { namespace Enyx {

	boost::shared_ptr< ::LiveMarketDataSource> CreateEnyxMarketDataSource() {
		return boost::shared_ptr< ::LiveMarketDataSource>(new MarketData);
	}

} } }
