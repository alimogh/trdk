/*******************************************************************************
 *   Created: 2017/10/15 22:27:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"

#ifdef _DEBUG
#pragma comment(lib, "odb-d-2.4-vc15.lib")
#pragma comment(lib, "odb-sqlite-d-2.4-vc15.lib")
#pragma comment(lib, "odb-qt5-d-2.4-vc15.lib")
#pragma comment(lib, "odb-boost-d-2.4-vc15.lib")
#else
#pragma comment(lib, "odb-2.4-vc15.lib")
#pragma comment(lib, "odb-sqlite-2.4-vc15.lib")
#pragma comment(lib, "odb-qt5-2.4-vc15.lib")
#pragma comment(lib, "odb-boost-2.4-vc15.lib")
#endif