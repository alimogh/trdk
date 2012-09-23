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

class trader__FirstUpdate {
	xsd__positiveInteger price;
	xsd__positiveInteger qty;
	xsd__boolean isBuy;
};

//gsoap trader service method-action: GetFirstUpdate "urn:#getFirstUpdate"
int trader__GetFirstUpdate(
		xsd__string symbol,
		std::list<trader__FirstUpdate> &getFirstUpdateResult);

//////////////////////////////////////////////////////////////////////////
