/**************************************************************************
 *   Created: 2012/07/08 04:47:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Defaults.hpp"

namespace fs = boost::filesystem;

fs::path Defaults::GetLogFilePath() {
	return fs::path("Logs");
}

fs::path Defaults::GetMarketDataLogDir() {
	auto result = GetLogFilePath();
	result /= "MarketData";
	return result;
}

fs::path Defaults::GetPositionsLogDir() {
	auto result = GetLogFilePath();
	result /= "Positions";
	return result;
}
