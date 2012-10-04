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

int trader__GetLastTrades(
			soap *soap,
			std::string symbol,
			std::string exchange,
			std::list<trader__Trade> &result) {
	reinterpret_cast<Service *>(soap->user)->GetLastTrades(symbol, exchange, result);
	return SOAP_OK;
}

int trader__GetParams(
			soap *soap,
			std::string symbol,
			std::string exchange,
			trader__ExchangeBook &result) {
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

int trader__OrderBuy(
			soap *soap,
			std::string symbol,
			ULONG64 price,
			ULONG64 qty,
			std::string *result) {
	reinterpret_cast<Service *>(soap->user)->OrderBuy(
		symbol,
		price,
		Trader::Security::Qty(qty),
		*result);
	return SOAP_OK;
}

int trader__OrderBuyMkt(
			soap *soap,
			std::string symbol,
			ULONG64 qty,
			std::string *result) {
	reinterpret_cast<Service *>(soap->user)->OrderBuyMkt(
		symbol,
		Trader::Security::Qty(qty),
		*result);
	return SOAP_OK;
}

int trader__OrderSell(
			soap *soap,
			std::string symbol,
			ULONG64 price,
			ULONG64 qty,
			std::string *result) {
	reinterpret_cast<Service *>(soap->user)->OrderSell(
		symbol,
		price,
		Trader::Security::Qty(qty),
		*result);
	return SOAP_OK;
}

int trader__OrderSellMkt(
			soap *soap,
			std::string symbol,
			ULONG64 qty,
			std::string *result) {
	reinterpret_cast<Service *>(soap->user)->OrderSellMkt(
		symbol,
		Trader::Security::Qty(qty),
		*result);
	return SOAP_OK;
}

int trader__GetPositionInfo(
			soap *soap,
			std::string symbol,
			trader__Position *result) {
	reinterpret_cast<Service *>(soap->user)->GetPositionInfo(symbol, *result);
	return SOAP_OK;
}
