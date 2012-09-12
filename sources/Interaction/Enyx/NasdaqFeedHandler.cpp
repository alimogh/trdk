/**************************************************************************
 *   Created: 2012/09/13 00:13:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "NasdaqFeedHandler.hpp"

using namespace Trader::Interaction::Enyx;

NasdaqFeedHandler::~NasdaqFeedHandler() {
	//...//
}

void NasdaqFeedHandler::onAuctionMessage(NXFeedAuction * AuctionMsg) {
#if 0
    nasdaqustvitch41::NXFeedAuctionPotential * AuctionPotential =
            (nasdaqustvitch41::NXFeedAuctionPotential*) AuctionMsg;

    switch (AuctionMsg->getSubType()) {
    case NXFEED_SUBTYPE_AUCTION_POTENTIAL:
        // generated when:
        //      * MessageType = "I" Net Order Imbalance Indicator
        //      => instrument filtering
        std::cout << *AuctionPotential << std::endl;
        break;
    default:
        std::cerr
                << "WARNING : Nasdaq Support should not produce this Auction subtype : "
                << AuctionMsg->getSubType() << std::endl;
        break;
    }
#endif
}

void NasdaqFeedHandler::onMiscMessage(NXFeedMisc * MiscMsg) {
#if 0
    NXFeedMiscTime * MiscTimeMsg = (NXFeedMiscTime*) MiscMsg;

    nasdaqustvitch41::NXFeedMiscStockDescription * MiscStockDescriptionMsg = (nasdaqustvitch41::NXFeedMiscStockDescription*) MiscMsg;
    nasdaqustvitch41::NXFeedInputPacketInfo * InputPacketInfoMsg = (nasdaqustvitch41::NXFeedInputPacketInfo*) MiscMsg;

    NXFeedMiscHeartbeat * MiscHeartbeatMsg = (NXFeedMiscHeartbeat*) MiscMsg;

    switch (MiscMsg->getSubType()) {
        // generated when:
        //      * Message Type = "T" Time Stamp - Second
    case NXFEED_SUBTYPE_MISC_TIME:
        std::cout << *MiscTimeMsg << std::endl;
        break;
        // generated when:
        //      * MessageType = "R" Stock Directory
    case NXFEED_SUBTYPE_MISC_STOCK_DESCRIPTION:
        std::cout << *MiscStockDescriptionMsg << std::endl;
        break;
        // generated at every start of packets
    case NXFEED_SUBTYPE_MISC_INPUT_PKT_INFO:
        std::cout << *InputPacketInfoMsg << std::endl;

        //We add a sleep here to slow down the incremental thread. This will make sure that the recovery will ends even with small capture files
        // WARNING, this is for simulation purpose only, don't use this usleep in production!
        usleep(10);
        break;
        // generated when a input packet contained 0 messages
    case NXFEED_SUBTYPE_MISC_HEARTBEAT:
        std::cout << *MiscHeartbeatMsg << std::endl;
        break;
        // generated when a change of session occur
    case NXFEED_SUBTYPE_MISC_SET_SEQNUM:
        std::cout << *MiscHeartbeatMsg << std::endl;
        break;
    default:
        std::cerr
                << "WARNING : Nasdaq Support should not produce this Misc subtype :"
                << MiscMsg->getSubType() << std::endl;
        break;
    }
#endif
}

void NasdaqFeedHandler::onExtraMessage(NXFeedExtra * ExtraMsg) {
#if 0
    switch (ExtraMsg->getSubType()) {
    // generated when:
        //      * MessageType = "Y" Reg SHO Short Sale Price Test Restricted Indicator
        //      * MessageType = "L" Market Participant Position
        //      => instrument filtering
    case NXFEED_SUBTYPE_EXTRA_INSTRUMENT:
        this->onExtraInstrMessage((NXFeedExtraInstr*) ExtraMsg);
        break;
    default:
        std::cerr
                << "NXFEED_TYPE_EXTRA FAIL received  message not supported by Nasdaq: "
                << ExtraMsg->getSubType() << std::endl;
        break;
    }
#endif
}

void NasdaqFeedHandler::onSystemInfoMessage(NXFeedSystemInfo * SystemInfoMsg) {
#if 0
    NXFeedSystemInfoMissingMd * SystemInfoMissingMdMsg      = (NXFeedSystemInfoMissingMd*) SystemInfoMsg;

    switch(SystemInfoMsg->getSubType() ) {
    // generated when:
        //      Recovery of Gapfilling are not enable and the MDA doesn't manage to repair the feed
    case NXFEED_SUBTYPE_SYSTEM_INFO_MISSING_MD_MSG:
        std::cout << *SystemInfoMissingMdMsg << std::endl;
        break;
    default:
        std::cout << "NXFEED_TYPE_SYSTEM_INFO FAIL received status message not supported by NXfeed: " << SystemInfoMsg->getSubType() << std::endl;
        break;
    }
#endif
}
void NasdaqFeedHandler::onStatusMessage(NXFeedStatus * status) {
#if 0
    nasdaqustvitch41::NXFeedInstrumentStatus * instr_status = (nasdaqustvitch41::NXFeedInstrumentStatus *) status;
    nasdaqustvitch41::NXFeedMarketStatus * market_status = (nasdaqustvitch41::NXFeedMarketStatus *) status;

    switch (status->getSubType()) {
    // generated when:
        //      * MessageType = "H" Stock Trading Action
        //      => instrument filtering
    case NXFEED_SUBTYPE_STATUS_INSTRUMENT:
        std::cout << *instr_status << std::endl;
        break;
        // generated when:
        //      * MessageType = "S" System Event Message
    case NXFEED_SUBTYPE_STATUS_MARKET :
        std::cout << *market_status << std::endl;
        break;
    default:
        std::cerr
                << "WARNING : Nasdaq Support should not produce this Status subtype : "
                << status->getSubType() << std::endl;
        break;
    }
#endif
}

void NasdaqFeedHandler::onOrderMessage(NXFeedOrder * OrderMsg) {

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

void NasdaqFeedHandler::onTradeMessage(NXFeedTrade * tradeMsg) {
#if 0
    NXFeedTradeReport * tradeReport = (NXFeedTradeReport*) tradeMsg;
    NXFeedTradeBreak * tradeBreak = (NXFeedTradeBreak*) tradeMsg;

    switch (tradeMsg->getSubType()) {
    // generated when:
        //      * MessageType = "P" Trade message non cross
        //              - tradeReport->getTradeType() = NXBUS_TRADE_TYPE_DAY_NOT_CROSS
        //      * MessageType = "Q" Cross Trade message
        //              - tradeReport->getTradeType() depend on Cross Type field
        //       => instrument filtering
    case NXFEED_SUBTYPE_TRADE_REPORT:
        std::cout << *tradeReport << std::endl;
        break;
    // generated when:
        //      * MessageType = "B" Broken Trade Message
    case NXFEED_SUBTYPE_TRADE_BREAK:
        std::cout << *tradeBreak << std::endl;
        break;
    default:
        std::cerr
                << "WARNING : Nasdaq Support should not produce this Trade message subtype : "
                << tradeMsg->getSubType() << std::endl;
        break;
    }
#endif
}

void NasdaqFeedHandler::onExtraInstrMessage(NXFeedExtraInstr * ExtraInstrMsg) {
#if 0
    nasdaqustvitch41::NXFeedExtraInstrMarketParticipantPosition * MarketParticipantPos = (nasdaqustvitch41::NXFeedExtraInstrMarketParticipantPosition*) ExtraInstrMsg;
    nasdaqustvitch41::NXFeedExtraInstrRegSHO * RegSHO = (nasdaqustvitch41::NXFeedExtraInstrRegSHO*) ExtraInstrMsg;

    switch (ExtraInstrMsg->getExtraType()) {
    // generated when:
        //      * MessageType = "Y" Reg SHO Short Sale Price Test Restricted Indicator
    case NXFEEDNASDAQUSTVITCH41_REG_SHO:
        std::cout << *RegSHO << std::endl;
        break;
    // generated when:
        //      * MessageType = "L" Market Participant Position
    case NXFEEDNASDAQUSTVITCH41_MARKET_PARTICIPANT_POSITION:
        std::cout << *MarketParticipantPos << std::endl;
        break;

    default:
        std::cout
                << "WARNING : Nasdaq Support should not produce this Instrument Extra message type : "
                << ExtraInstrMsg->getExtraType() << std::endl;
        break;
    }
#endif
}
