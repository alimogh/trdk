/*******************************************************************************
 *   Created: 2017/07/23 12:51:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#ifdef TRDK_STRATEGY_MRIGESHKEJRIWAL
#ifdef BOOST_WINDOWS
#define TRDK_STRATEGY_MRIGESHKEJRIWAL_API __declspec(dllexport)
#else
#define TRDK_STRATEGY_MRIGESHKEJRIWAL_API
#endif
#else
#ifdef BOOST_WINDOWS
#define TRDK_STRATEGY_MRIGESHKEJRIWAL_API __declspec(dllimport)
#else
#define TRDK_STRATEGY_MRIGESHKEJRIWAL_API
#endif
#endif
