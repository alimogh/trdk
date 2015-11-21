/**************************************************************************
 *   Created: 2015/07/08 02:43:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#if defined(_DEBUG)
#	pragma comment(lib, "libprotobuf_x64_vs120_dbg.lib")
#elif defined(_TEST)
#	pragma comment(lib, "libprotobuf_x64_vs120_test.lib")
#else
#	pragma comment(lib, "libprotobuf_x64_vs120.lib")
#endif
