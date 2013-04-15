/**************************************************************************
 *   Created: 2012/09/19 23:38:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Service.hpp"

using namespace trdk;
using namespace trdk::Gateway;
using namespace trdk::Lib;

#ifdef BOOST_WINDOWS
#	define TRADER_GATEWAY_SERVICE_API
#else
#	define TRADER_GATEWAY_SERVICE_API extern "C"
#endif

TRADER_GATEWAY_SERVICE_API boost::shared_ptr<Observer> CreateGateway(
			Context &context,
			const std::string &tag,
			const Observer::NotifyList &notifyList,
			const IniFileSectionRef &configuration) {
	return boost::shared_ptr<Observer>(
		new Gateway::Service(context, tag, notifyList, configuration));
}
