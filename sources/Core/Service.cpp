/**************************************************************************
 *   Created: 2012/11/03 18:26:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Service.hpp"

using namespace Trader;

Service::Service(const std::string &tag, boost::shared_ptr<Security> security)
		: SecurityAlgo(tag, security) {
	//...//
}

Service::~Service() {
	//...//
}
