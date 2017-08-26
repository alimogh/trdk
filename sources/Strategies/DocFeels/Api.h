/*******************************************************************************
 *   Created: 2017/08/20 14:14:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#ifdef TRDK_STRATEGY_DOCFEELS
#ifdef BOOST_WINDOWS
#define TRDK_STRATEGY_DOCFEELS_API __declspec(dllexport)
#else
#define TRDK_STRATEGY_DOCFEELS_API
#endif
#else
#ifdef BOOST_WINDOWS
#define TRDK_STRATEGY_DOCFEELS_API __declspec(dllimport)
#else
#define TRDK_STRATEGY_DOCFEELS_API
#endif
#endif
