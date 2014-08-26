/**************************************************************************
 *   Created: 2014/08/15 01:40:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#ifdef BOOST_WINDOWS
#	define TRDK_STRATEGY_FXMB_API
#else
#	define TRDK_STRATEGY_FXMB_API extern "C"
#endif
