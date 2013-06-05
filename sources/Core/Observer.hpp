/**************************************************************************
 *   Created: 2012/09/19 23:57:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Consumer.hpp"
#include "Api.h"

namespace trdk {

	class TRDK_CORE_API Observer : public trdk::Consumer {

	public:

		Observer(
				trdk::Context &,
				const std::string &name,
				const std::string &tag);
		~Observer();

	public:

		void RaiseLevel1UpdateEvent(Security &);
		void RaiseLevel1TickEvent(
					trdk::Security &,
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &);
		void RaiseNewTradeEvent(
					trdk::Security &,
					const boost::posix_time::ptime &,
					trdk::ScaledPrice,
					trdk::Qty,
					trdk::OrderSide);
		void RaiseServiceDataUpdateEvent(const trdk::Service &);

	};

}
