/**************************************************************************
 *   Created: 2012/08/25 21:28:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#if defined(_MSC_VER)
#	ifdef TRDK_CORE
#		define TRDK_CORE_API
#	else
#		define TRDK_CORE_API
#	endif
#else
#	define TRDK_CORE_API
#endif
