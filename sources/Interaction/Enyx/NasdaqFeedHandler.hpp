/**************************************************************************
 *   Created: 2012/09/13 00:12:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

namespace Trader {  namespace Interaction { namespace Enyx {

	class NasdaqFeedHandler: public NXFeedHandler {

	public:
		
		virtual ~NasdaqFeedHandler();

	public:

		virtual void onAuctionMessage(NXFeedAuction *);
		virtual void onMiscMessage(NXFeedMisc *);
		virtual void onExtraMessage(NXFeedExtra *);
		virtual void onSystemInfoMessage(NXFeedSystemInfo *);
		virtual void onStatusMessage(NXFeedStatus *);
		virtual void onOrderMessage(NXFeedOrder *);
		virtual void onTradeMessage(NXFeedTrade *);
		virtual void onExtraInstrMessage(NXFeedExtraInstr *);

	};

} } }
