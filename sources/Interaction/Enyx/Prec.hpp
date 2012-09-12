/**************************************************************************
 *   Created: 2012/09/11 01:43:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Common/Assert.hpp"

#ifndef BOOST_WINDOWS
#	include <EnyxMD.h>
#	include <dictionary/Dictionary.h>
#	include <EnyxMDPcapInterface.h>
#	include <NXFeedHandler.h>
#	include <NXOrderManager.h>
#	include <exchanges/nasdaq_us_totalview_itch41/NasdaqUSTVITCH41NXFeedExtra.h>
#endif

#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/shared_ptr.hpp>
#include "Common/DisableBoostWarningsEnd.h"

#include "Common/Common.hpp"

#include "Common/Assert.hpp"

#define TRADER_ENYX "Enyx"
#define TRADER_ENYX_LOG_PREFFIX TRADER_ENYX ": "
