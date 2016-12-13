/**************************************************************************
 *   Created: 2016/12/12 22:26:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once
 
#ifdef TRDK_STRATEGY_INTRADAYTREND
#	ifdef BOOST_WINDOWS
#		define TRDK_STRATEGY_INTRADAYTREND_API __declspec(dllexport)
#	else
#		define TRDK_STRATEGY_INTRADAYTREND_API
#	endif
#else
#	ifdef BOOST_WINDOWS
#		define TRDK_STRATEGY_INTRADAYTREND_API __declspec(dllimport)
#	else
#		define TRDK_STRATEGY_INTRADAYTREND_API
#	endif
#endif
