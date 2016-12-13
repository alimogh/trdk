/**************************************************************************
 *   Created: 2016/02/08 18:56:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#ifdef BOOST_WINDOWS
#	define TRDK_STRATEGY_TEST_API
#else
#	define TRDK_STRATEGY_TEST_API extern "C"
#endif
