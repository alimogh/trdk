/**************************************************************************
 *   Created: 2013/05/01 02:31:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"

#ifdef _DEBUG
#pragma comment(lib, "ssleay32MTd.lib")
#pragma comment(lib, "libeay32MTd.lib")
#else
#pragma comment(lib, "ssleay32MT.lib")
#pragma comment(lib, "libeay32MT.lib")
#endif  // _DEBUG
