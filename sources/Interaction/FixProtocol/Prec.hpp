/*******************************************************************************
 *   Created: 2017/09/19 19:24:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Common/Common.hpp"
#include "Core/Settings.hpp"
#include "FixProtocolApi.h"
#include "FixProtocolFwd.hpp"
#include <boost/thread/recursive_mutex.hpp>

#include "Common/NetworkStreamClient.hpp"
#include "Common/NetworkStreamClientService.hpp"