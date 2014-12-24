/**************************************************************************
 *   Created: 2014/11/29 14:22:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#ifdef BOOST_WINDOWS
#	define TRDK_INTERACTION_LOGREPLAY_API
#else
#	define TRDK_INTERACTION_LOGREPLAY_API extern "C"
#endif
