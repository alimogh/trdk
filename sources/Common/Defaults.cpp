/**************************************************************************
 *   Created: 2012/07/08 04:47:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Defaults.hpp"
#include "Util.hpp"

using namespace Trader;
using namespace Trader::Lib;
namespace fs = boost::filesystem;

fs::path Defaults::GetLogFilePath() {
	return GetExeWorkingDir() / fs::path("logs");
}

fs::path Defaults::GetMarketDataLogDir() {
	auto result = GetLogFilePath();
	result /= "MarketData";
	return result;
}

fs::path Defaults::GetBarsDataLogDir() {
	auto result = GetLogFilePath();
	result /= "Bars";
	return result;
}

fs::path Defaults::GetPositionsLogDir() {
	auto result = GetLogFilePath();
	result /= "Positions";
	return result;
}

fs::path Defaults::GetRawDataLogDir() {
	auto result = GetLogFilePath();
	result /= "Raw";
	return result;
}
