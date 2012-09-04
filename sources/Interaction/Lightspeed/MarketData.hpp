/**************************************************************************
 *   Created: 2012/08/28 20:38:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"

namespace Trader {  namespace Interaction { namespace Lightspeed {

	class MarketData : public ::LiveMarketDataSource {

	public:

		MarketData() {
			//...//
		}

		virtual ~MarketData() {
			//...//
		}

	public:

		virtual void Connect(const Settings &) {
			//...//
		}

	public:

		virtual void SubscribeToMarketDataLevel1(boost::shared_ptr<Security>) const {
			//...//
		}

		virtual void SubscribeToMarketDataLevel2(boost::shared_ptr<Security>) const {
			//...//
		}

	};

} } }
