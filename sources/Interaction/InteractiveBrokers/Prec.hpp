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

#include "Common/Common.hpp"
#include "Fwd.hpp"
#pragma warning(push, 3)
#pragma warning(disable : 4800)
#include "Api/CommissionReport.h"
#include "Api/Contract.h"
#include "Api/EPosixClientSocket.h"
#include "Api/EWrapper.h"
#include "Api/Execution.h"
#include "Api/Order.h"
#pragma warning(pop)
#include <boost/algorithm/string.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <WinSock2.h>
#include <Windows.h>
#include <concrt.h>

#ifdef BOOST_WINDOWS
#undef SendMessage
#undef ERROR
#undef GetCurrentTime
#endif
