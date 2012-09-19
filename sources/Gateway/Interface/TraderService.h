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

//////////////////////////////////////////////////////////////////////////

class trader__State {
public:
    xsd__positiveInteger revision;
};

//gsoap trader service method-action: GetState "urn:#getState"
int trader__GetState(trader__State &getStateResult);

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

class trader__MarketDataParameter {
public:
    xsd__positiveInteger qty;
    xsd__positiveInteger price;
};

class trader__MarketData {
public:
	unsigned int securityId;
	trader__MarketDataParameter ask;
    trader__MarketDataParameter bid;
    trader__MarketDataParameter last;
};

//gsoap trader service method-action: trader__GetCurrentMarketData "urn:#getCurrentMarketData"
int trader__GetCurrentMarketData(
        xsd__positiveInteger &revisionResult,
        std::list<trader__MarketData> &currentMarketDataResult);

//////////////////////////////////////////////////////////////////////////
