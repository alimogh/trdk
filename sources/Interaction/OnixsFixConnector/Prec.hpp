/**************************************************************************
 *   Created: 2014/08/09 12:10:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/Assert.hpp"

#include "Common/DisableBoostWarningsBegin.h"
#	include	<boost/algorithm/string.hpp>
#	include	<boost/scoped_ptr.hpp>
#	include	<boost/noncopyable.hpp>
#	ifndef BOOST_WINDOWS
#		include <boost/thread.hpp>
#	endif
#include "Common/DisableBoostWarningsEnd.h"

#include "DisableOnixsFixEngineWarningsBegin.h"
#	include <OnixS/FIXEngine.h>
#include "DisableOnixsFixEngineWarningsEnd.h"

#include "Common/Common.hpp"

#ifdef BOOST_WINDOWS
#	include <concrt.h>
#endif

#include "Api.h"
#include "Fwd.hpp"

#include "Common/Assert.hpp"
