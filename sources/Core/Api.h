/**************************************************************************
 *   Created: 2012/08/25 21:28:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#if defined(_MSC_VER)
#	ifdef TRADER_CORE
#		define TRADER_CORE_API __declspec(dllexport)
#	else
#		define TRADER_CORE_API __declspec(dllimport)
#	endif
#else
#	define TRADER_CORE_API
#endif
