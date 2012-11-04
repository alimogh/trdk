/**************************************************************************
 *   Created: 2012/09/19 23:38:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Service.hpp"

using namespace Trader;
using namespace Trader::Gateway;
using namespace Trader::Lib;

#ifdef BOOST_WINDOWS
#	define TRADER_GATEWAY_SERVICE_API
#else
#	define TRADER_GATEWAY_SERVICE_API extern "C"
#endif

TRADER_GATEWAY_SERVICE_API boost::shared_ptr<Observer> CreateGateway(
			const std::string &tag,
			const Observer::NotifyList &notifyList,
			boost::shared_ptr<Trader::TradeSystem> tradeSystem,
			const IniFile &ini,
			const std::string &section) {
	return boost::shared_ptr<Observer>(
		new Gateway::Service(tag, notifyList, tradeSystem, ini, section));
}
