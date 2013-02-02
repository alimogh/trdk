/**************************************************************************
 *   Created: 2013/01/31 00:49:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#if defined(_MSC_VER)
#	ifdef TRADER_ENGINE
#		define TRADER_ENGINE_API __declspec(dllexport)
#	else
#		define TRADER_ENGINE_API __declspec(dllimport)
#	endif
#else
#	define TRADER_ENGINE_API
#endif
