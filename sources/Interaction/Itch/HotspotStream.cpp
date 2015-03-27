/**************************************************************************
 *   Created: 2015/03/17 23:58:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Stream.hpp"

TRDK_INTERACTION_ITCH_API
boost::shared_ptr<trdk::MarketDataSource>
CreateHotspotStream(
		size_t index,
		trdk::Context &context,
		const std::string &tag,
		const trdk::Lib::IniSectionRef &configuration) {
	return boost::shared_ptr<trdk::MarketDataSource>(
		new trdk::Interaction::Itch::Stream(
			index,
			context,
			tag,
			configuration));
}
