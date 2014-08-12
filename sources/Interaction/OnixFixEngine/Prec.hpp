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

#include "Fwd.hpp"
#include "Api.h"

#include "Common/DisableBoostWarningsBegin.h"
#include	<boost/algorithm/string.hpp>
#include	<boost/scoped_ptr.hpp>
#include	<boost/noncopyable.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include "DisableOnixFixEngineWarningsBegin.h"
#	include <OnixS/FIXEngine.h>
#include "DisableOnixFixEngineWarningsEnd.h"

#include "Common/Common.hpp"

#include "Common/Assert.hpp"
