/*******************************************************************************
 *   Created: 2017/10/22 17:11:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#if defined(_MSC_VER)
#ifdef TRDK_STRATEGY_ARBITRATIONADVISER
#define TRDK_STRATEGY_ARBITRATIONADVISER_API __declspec(dllexport)
#else
#define TRDK_STRATEGY_ARBITRATIONADVISER_API __declspec(dllimport)
#endif
#endif

#if !defined(TRDK_STRATEGY_ARBITRATIONADVISER_API)
#define TRDK_STRATEGY_ARBITRATIONADVISER_API
#endif
