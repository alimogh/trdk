/**************************************************************************
 *   Created: 2012/07/09 12:47:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include <boost/filesystem.hpp>

struct Defaults {

	static boost::filesystem::path GetLogFilePath();

	static boost::filesystem::path GetIqFeedLogFilePath();

	static boost::filesystem::path GetMarketDataLogDir();

};
