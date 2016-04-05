/**************************************************************************
 *   Created: 2016/04/05 08:21:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#ifdef BOOST_WINDOWS
#	define TRDK_STRATEGY_GADM_API
#else
#	define TRDK_STRATEGY_GADM_API extern "C"
#endif
