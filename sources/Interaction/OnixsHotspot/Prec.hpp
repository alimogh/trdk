/**************************************************************************
 *   Created: 2014/10/16 00:28:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/Assert.hpp"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable: 4512)
#endif
#include <OnixS/HotSpot/HotspotItch.h>
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#include "Common/Common.hpp"

#ifdef BOOST_WINDOWS
#	include <concrt.h>
#endif

#include "Api.h"

#include "Common/Assert.hpp"
