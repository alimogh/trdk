/**************************************************************************
 *   Created: 2014/08/12 23:51:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#ifdef BOOST_WINDOWS
#	define TRDK_INTERACTION_ONIXFIXENGINE_API
#else
#	define TRDK_INTERACTION_ONIXFIXENGINE_API extern "C"
#endif
