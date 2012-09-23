/**************************************************************************
 *   Created: 2012/09/20 21:20:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Service.hpp"

using namespace Trader::Gateway;

int trader__GetSecurityList(soap *soap, std::list<trader__Security >&result) {
	reinterpret_cast<Service *>(soap->user)->GetSecurityList(result);
	return SOAP_OK;
}

int trader__GetFirstUpdate(
			soap *soap,
			std::string symbol,
			std::list<trader__FirstUpdate> &result) {
	reinterpret_cast<Service *>(soap->user)->GetFirstUpdate(symbol, result);
	return SOAP_OK;
}
