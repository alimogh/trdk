/**************************************************************************
 *   Created: 2016/04/05 07:27:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#ifdef BOOST_WINDOWS
#	define TRDK_INTERACTION_INTERACTIVEBROKERS_API
#else
#	define TRDK_INTERACTION_INTERACTIVEBROKERS_API extern "C"
#endif
