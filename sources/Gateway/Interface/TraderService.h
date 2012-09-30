/**************************************************************************
 *   Created: 2012/09/19 22:46:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#import "stllist.h"

//gsoap trader service name: TraderService
//gsoap trader service style: rpc
//gsoap trader service namespace: http://palchukovsky.com/services/schemas/trader/1.0.wsdl
//gsoap trader schema namespace: http://palchukovsky.com/services/schemas/trader/1.0.xsd
//gsoap trader service location: No

//////////////////////////////////////////////////////////////////////////

typedef unsigned long long xsd__positiveInteger;
typedef std::string xsd__string;
typedef bool xsd__boolean;

//////////////////////////////////////////////////////////////////////////

class trader__Security {
public:
    xsd__positiveInteger id;
	xsd__string symbol;
    xsd__positiveInteger scale;
};

//gsoap trader service method-action: GetSecurityList "urn:#getSecurityList"
int trader__GetSecurityList(std::list<trader__Security> &getSecurityListResult);

//////////////////////////////////////////////////////////////////////////

class trader__Param {
	xsd__positiveInteger price;
	xsd__positiveInteger qty;
};

class trader__ExchangeParams {
	trader__Param ask;
	trader__Param bid;
};

class trader__Time {
	xsd__positiveInteger date;
	xsd__positiveInteger time;
};

//////////////////////////////////////////////////////////////////////////

class trader__Order {
	trader__Time time;
	trader__Param param;
	xsd__boolean isBuy;
};
typedef std::list<trader__Order> trader__OrderList;

//gsoap trader service method-action: GetLastOrders "urn:#getLastOrders"
int trader__GetLastOrders(
		xsd__string symbol,
		xsd__string exchange,
		trader__OrderList &getLastOrdersResult);

//////////////////////////////////////////////////////////////////////////

//gsoap trader service method-action: GetParams "urn:#getParams"
int trader__GetParams(
		xsd__string symbol,
		xsd__string exchange,
		trader__ExchangeParams &getParamsResult);


//////////////////////////////////////////////////////////////////////////

class trader__CommonParams {
	trader__Param last;
	trader__ExchangeParams best;
	xsd__positiveInteger volumeTraded;
};

//gsoap trader service method-action: GetCommonParams "urn:#getCommonParams"
int trader__GetCommonParams(
		xsd__string symbol,
		trader__CommonParams &getCommonParamsResult);

//////////////////////////////////////////////////////////////////////////
