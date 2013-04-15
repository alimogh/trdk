/**************************************************************************
 *   Created: 2012/09/19 22:46:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
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
typedef char xsd__byte;

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

class trader__Trade {
	trader__Time time;
	trader__Param param;
	xsd__boolean isBuy;
};
typedef std::list<trader__Trade> trader__TradeList;

//gsoap trader service method-action: GetLastTrades "urn:#getLastTrades"
int trader__GetLastTrades(
		xsd__string symbol,
		xsd__string exchange,
		trader__TradeList &getLastTradesResult);

//////////////////////////////////////////////////////////////////////////

class trader__ExchangeBook {
	trader__ExchangeParams params1;
	trader__ExchangeParams params2;
};

//gsoap trader service method-action: GetParams "urn:#getParams"
int trader__GetParams(
		xsd__string symbol,
		xsd__string exchange,
		trader__ExchangeBook &getParamsResult);


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

//gsoap trader service method-action: OrderBuy "urn:#orderBuy"
int trader__OrderBuy(
		xsd__string symbol,
		xsd__string venue,
		xsd__positiveInteger price,
		xsd__positiveInteger qty,
		xsd__string *orderBuyResult);
//gsoap trader service method-action: OrderBuyMkt "urn:#orderBuyMkt"
int trader__OrderBuyMkt(
		xsd__string symbol,
		xsd__string venue,
		xsd__positiveInteger qty,
		xsd__string *orderBuyResult);

//gsoap trader service method-action: OrderSell "urn:#orderSell"
int trader__OrderSell(
		xsd__string symbol,
		xsd__string venue,
		xsd__positiveInteger price,
		xsd__positiveInteger qty,
		xsd__string *orderSellResult);
//gsoap trader service method-action: OrderSellMkt "urn:#orderSellMkt"
int trader__OrderSellMkt(
		xsd__string symbol,
		xsd__string venue,
		xsd__positiveInteger qty,
		xsd__string *orderSellResult);

//////////////////////////////////////////////////////////////////////////

class trader__Position {
	xsd__byte side;
	xsd__positiveInteger price;
	xsd__positiveInteger qty;
};

//gsoap trader service method-action: GetPositionInfo "urn:#getPositionInfo"
int trader__GetPositionInfo(xsd__string symbol, trader__Position *positionResult);

//////////////////////////////////////////////////////////////////////////
