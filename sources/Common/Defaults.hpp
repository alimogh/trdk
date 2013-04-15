/**************************************************************************
 *   Created: 2012/07/09 12:47:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include <boost/filesystem.hpp>

namespace trdk {

	struct Defaults {

		static boost::filesystem::path GetLogFilePath();

		static boost::filesystem::path GetMarketDataLogDir();
		
		static boost::filesystem::path GetBarsDataLogDir();

		static boost::filesystem::path GetPositionsLogDir();

		static boost::filesystem::path GetRawDataLogDir();

	};

}
