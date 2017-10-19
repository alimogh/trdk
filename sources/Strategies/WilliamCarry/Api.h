/*******************************************************************************
 *   Created: 2017/10/14 00:21:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#if defined(_MSC_VER)
#ifdef TRDK_STRATEGY_WILLIAMCARRY
#define TRDK_STRATEGY_WILLIAMCARRY_API __declspec(dllexport)
#else
#define TRDK_STRATEGY_WILLIAMCARRY_API __declspec(dllimport)
#endif
#endif

#if !defined(TRDK_STRATEGY_WILLIAMCARRY_API)
#define TRDK_STRATEGY_WILLIAMCARRY_API
#endif
