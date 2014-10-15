/**************************************************************************
 *   Created: 2014/10/16 00:34:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#ifdef BOOST_WINDOWS
#	define TRDK_INTERACTION_ONIXSHOTSPOT_API
#else
#	define TRDK_INTERACTION_ONIXSHOTSPOT_API extern "C"
#endif
