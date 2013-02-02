/**************************************************************************
 *   Created: 2012/10/27 14:21:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "CsvMarketDataSource.hpp"

boost::shared_ptr<Trader::MarketDataSource> CreateMarketDataSource(
			const Trader::Lib::IniFileSectionRef &configuration,
			Trader::Context::Log &log) {
 	return boost::shared_ptr<Trader::MarketDataSource>(
 		new Trader::Interaction::Csv::MarketDataSource(configuration, log));
}
