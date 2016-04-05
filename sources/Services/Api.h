/**************************************************************************
 *   Created: 2012/11/20 22:21:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#if defined(DISTRIBUTION_STANDALONE)
#	if defined(_MSC_VER)
#		ifdef TRDK_SERVICES
#			define TRDK_SERVICES_API __declspec(dllexport)
#		else
#			define TRDK_SERVICES_API __declspec(dllimport)
#		endif
#	endif
#endif

#if !defined(TRDK_SERVICES_API)
#	define TRDK_SERVICES_API
#endif
