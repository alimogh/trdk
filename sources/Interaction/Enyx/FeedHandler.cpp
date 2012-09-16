/**************************************************************************
 *   Created: 2012/09/13 00:13:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "FeedHandler.hpp"
#include "Core/Security.hpp"

using namespace Trader::Interaction::Enyx;

////////////////////////////////////////////////////////////////////////////////

FeedHandler::SubscribedSecurity::SubscribedSecurity(boost::shared_ptr<Security> security)
		: m_security(security) {
	//...//
}

const std::string & FeedHandler::SubscribedSecurity::GetExchange() const {
	return m_security->GetPrimaryExchange();
}

const std::string & FeedHandler::SubscribedSecurity::GetSymbol() const {
	return m_security->GetFullSymbol();
}

////////////////////////////////////////////////////////////////////////////////

FeedHandler::~FeedHandler() {
	//...//
}

void FeedHandler::onOrderMessage(NXFeedOrder * OrderMsg) {

	nasdaqustvitch41::NXFeedOrderAdd * add_order = (nasdaqustvitch41::NXFeedOrderAdd *) OrderMsg;
    NXFeedOrderExecute * exe_order = (NXFeedOrderExecute *) OrderMsg;
    nasdaqustvitch41::NXFeedOrderExeWithPrice * exe_with_price_order = (nasdaqustvitch41::NXFeedOrderExeWithPrice *) OrderMsg;
    NXFeedOrderReduce* reduce_order     =  (NXFeedOrderReduce *) OrderMsg;
    NXFeedOrderDelete* delete_order     =  (NXFeedOrderDelete *) OrderMsg;
    NXFeedOrderReplace* replace_order   =  (NXFeedOrderReplace *) OrderMsg;

	std::ostringstream oss;

    switch(OrderMsg->getSubType()) {
    // generated when:
        //      * MessageType = "A" Add order - No MPID Attribution
        //              - add_order->getAttribution() is equal to "NSDQ"
        //      * MessageType = "F" Add order with MPID Attribution
        //      => instrument filtering
    case  NXFEED_SUBTYPE_ORDER_ADD :
		oss << "New order added: " << *add_order;
        break;
    // generated when:
        //      * MessageType = "E" Order Executed Message
    case  NXFEED_SUBTYPE_ORDER_EXEC :
		oss << "Order exec: " << *exe_order;
        break;
        // generated when:
        //      * MessageType = "C" Order Executed with price Message
    case  NXFEED_SUBTYPE_ORDER_EXEC_PRICE  :
		oss << "Order exec with price: " << *exe_with_price_order;
        break;
    // generated when:
        //      * MessageType = "X" Order Cancel Message
    case  NXFEED_SUBTYPE_ORDER_REDUCE :
        return;
    // generated when:
        //      * MessageType = "D" Order Delete Message
    case  NXFEED_SUBTYPE_ORDER_DEL  :
		oss << "Order deleted: " << *delete_order;
        break;
    // generated when:
        //      * MessageType = "U" Order Replace Message
    case  NXFEED_SUBTYPE_ORDER_REPLACE  :
        return;
    default:
        Log::Error(TRADER_ENYX_LOG_PREFFIX "Nasdaq Support should not produce this Trade message subtype");
		return;
    }

	Log::Debug(TRADER_ENYX_LOG_PREFFIX "%1%", oss.str().c_str());

}

void FeedHandler::Subscribe(boost::shared_ptr<Security> security) const throw() {
	try {
		// not checking result: if already exists - good
		m_subscribtion.insert(SubscribedSecurity(security));
	} catch (...) {
		AssertFailNoException();
	}
}
