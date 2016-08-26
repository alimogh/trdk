/**************************************************************************
 *   Created: 2016/08/23 23:28:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#ifdef BOOST_WINDOWS
#	define TRDK_INTERACTION_DDFPLUS_API
#else
#	define TRDK_INTERACTION_DDFPLUS_API extern "C"
#endif
