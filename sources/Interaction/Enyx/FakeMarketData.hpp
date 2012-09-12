/**************************************************************************
 *   Created: 2012/09/11 18:52:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"

namespace Trader {  namespace Interaction { namespace Enyx {

	class FakeMarketData : public ::LiveMarketDataSource {

	public:

		FakeMarketData() {
			//...//
		}

		virtual ~FakeMarketData() {
			//...//
		}

	public:

		virtual void Connect(const IniFile &, const std::string &/*section*/) {
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
