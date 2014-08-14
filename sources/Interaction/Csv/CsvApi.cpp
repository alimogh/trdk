/**************************************************************************
 *   Created: 2012/10/27 14:21:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "CsvMarketDataSource.hpp"

boost::shared_ptr<trdk::MarketDataSource> CreateMarketDataSource(
			const std::string &tag,
			const trdk::Lib::IniSectionRef &configuration,
			trdk::Context::Log &log) {
 	return boost::shared_ptr<trdk::MarketDataSource>(
 		new trdk::Interaction::Csv::MarketDataSource(tag, configuration, log));
}
