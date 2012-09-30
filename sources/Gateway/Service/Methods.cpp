/**************************************************************************
 *   Created: 2012/09/20 21:20:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Service.hpp"
#include "../Interface/TraderService.nsmap"

using namespace Trader::Gateway;

int trader__GetSecurityList(soap *soap, std::list<trader__Security >&result) {
	reinterpret_cast<Service *>(soap->user)->GetSecurityList(result);
	return SOAP_OK;
}

int trader__GetLastOrders(
			soap *soap,
			std::string symbol,
			std::string exchange,
			trader__OrderList &result) {
	reinterpret_cast<Service *>(soap->user)->GetLastOrders(symbol, exchange, result);
	return SOAP_OK;
}

int trader__GetParams(
			soap *soap,
			std::string symbol,
			std::string exchange,
			trader__ExchangeParams &result) {
	reinterpret_cast<Service *>(soap->user)->GetParams(symbol, exchange, result);
	return SOAP_OK;
}

int trader__GetCommonParams(
			soap *soap,
			std::string symbol,
			trader__CommonParams &result) {
	reinterpret_cast<Service *>(soap->user)->GetCommonParams(symbol, result);
	return SOAP_OK;
}
