/**************************************************************************
 *   Created: 2016/11/20 20:23:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/
 
#pragma once

#ifdef TRDK_INTERACTION_IQFEED
#	ifdef BOOST_WINDOWS
#		define TRDK_INTERACTION_IQFEED_API __declspec(dllexport)
#	else
#		define TRDK_INTERACTION_IQFEED_API
#	endif
#else
#	ifdef BOOST_WINDOWS
#		define TRDK_INTERACTION_IQFEED_API __declspec(dllimport)
#	else
#		define TRDK_INTERACTION_IQFEED_API
#	endif
#endif
