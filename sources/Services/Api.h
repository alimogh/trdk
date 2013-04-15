/**************************************************************************
 *   Created: 2012/11/20 22:21:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#if defined(_MSC_VER)
#	ifdef TRADER_SERVICES
#		define TRADER_SERVICES_API __declspec(dllexport)
#	else
#		define TRADER_SERVICES_API __declspec(dllimport)
#	endif
#else
#	define TRADER_SERVICES_API
#endif
