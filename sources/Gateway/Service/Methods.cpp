/**************************************************************************
 *   Created: 2012/09/20 21:20:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "../Interface/soapStub.h"
#include "../Interface/TraderService.nsmap"

int trader__GetState(soap *, trader__State &result) {
	UseUnused(result);
	return SOAP_OK;
}

int trader__GetSecurityList(soap *, std::list<trader__Security >&result) {
	UseUnused(result);
	return SOAP_OK;
}

int trader__GetCurrentMarketData(soap *, std::list<trader__MarketData >&result) {
	UseUnused(result);
	return SOAP_OK;
}
