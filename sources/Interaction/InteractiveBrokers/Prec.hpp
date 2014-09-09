/**************************************************************************
 *   Created: 2012/07/09 14:36:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/Assert.hpp"

#pragma warning(push, 3)
#pragma warning(disable: 4800)
#include "Api/EWrapper.h"
#include "Api/Contract.h"
#include "Api/Order.h"
#include "Api/EPosixClientSocket.h"
#pragma warning(pop)

#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/thread.hpp>
#	include <boost/algorithm/string.hpp>
#	include <boost/multi_index_container.hpp>
#	include <boost/multi_index/composite_key.hpp>
#	include <boost/multi_index/mem_fun.hpp>
#	include <boost/multi_index/ordered_index.hpp>
#	include <boost/multi_index/hashed_index.hpp>
#	include <boost/multi_index/member.hpp>
#	include <boost/regex.hpp>
#	include <boost/math/special_functions/fpclassify.hpp>
#	include <boost/math/special_functions/round.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include <concrt.h>

#include <Windows.h>
#include <WinSock2.h>

#include "Common/Common.hpp"

#include "Common/Assert.hpp"
