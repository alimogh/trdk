/**************************************************************************
 *   Created: 2015/07/12 19:39:08
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
#		ifdef TRDK_DROPCOPY
#			define TRDK_DROPCOPY_API __declspec(dllexport)
#		else
#			define TRDK_DROPCOPY_API __declspec(dllimport)
#		endif
#	endif
#endif

#if !defined(TRDK_DROPCOPY_API)
#	define TRDK_DROPCOPY_API
#endif
