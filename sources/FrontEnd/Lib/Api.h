/*******************************************************************************
 *   Created: 2017/10/15 22:28:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#if defined(_MSC_VER)
#ifdef TRDK_FRONTEND_LIB
#define TRDK_FRONTEND_LIB_API __declspec(dllexport)
#else
#define TRDK_FRONTEND_LIB_API __declspec(dllimport)
#endif
#endif

#if !defined(TRDK_FRONTEND_LIB_API)
#define TRDK_FRONTEND_LIB_API
#endif
