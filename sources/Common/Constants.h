/**************************************************************************
 *   Created: 2013/09/05 17:33:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Lib { namespace Concurrency {
	enum Profile {
		PROFILE_RELAX,
		PROFILE_HFT,
		numberOfProfiles
	};
} } }

////////////////////////////////////////////////////////////////////////////////

#include "Version/Version.h"

////////////////////////////////////////////////////////////////////////////////

#if defined(_DEBUG) && !defined(_TEST) && !defined(NDEBUG)
#	define TRDK_CONCURRENCY_PROFILE TRDK_CONCURRENCY_PROFILE_DEBUG
#elif defined(_TEST) && defined(NDEBUG) && !defined(NTEST)
#	define TRDK_CONCURRENCY_PROFILE TRDK_CONCURRENCY_PROFILE_TEST
#elif defined(NDEBUG) && !defined(_DEBUG) && defined(NTEST) && !defined(_TEST)
#	define TRDK_CONCURRENCY_PROFILE TRDK_CONCURRENCY_PROFILE_RELEASE
#endif

////////////////////////////////////////////////////////////////////////////////

#define _STR(a) #a
#define _XSTR(a) _STR(a)
#define _WSTR(a) L#a
#define _XWSTR(a) _WSTR(a)

////////////////////////////////////////////////////////////////////////////////

#define TRDK_VERSION	\
	_XSTR(TRDK_VERSION_MAJOR_HIGH) \
	"." _XSTR(TRDK_VERSION_MAJOR_LOW) \
	"." _XSTR(TRDK_VERSION_MINOR_HIGH)
#define TRDK_VERSION_W	\
	_XWSTR(TRDK_VERSION_MAJOR_HIGH) \
	L"." _XWSTR(TRDK_VERSION_MAJOR_LOW) \
	L"." _XWSTR(TRDK_VERSION_MINOR_HIGH)

#define TRDK_VERSION_FULL \
	_XSTR(TRDK_VERSION_MAJOR_HIGH) \
	"." _XSTR(TRDK_VERSION_MAJOR_LOW) \
	"." _XSTR(TRDK_VERSION_MINOR_HIGH) \
	"." _XSTR(TRDK_VERSION_MINOR_LOW)
#define TRDK_VERSION_FULL_W	\
	_XWSTR(TRDK_VERSION_MAJOR_HIGH) \
	L"." _XWSTR(TRDK_VERSION_MAJOR_LOW) \
	L"." _XWSTR(TRDK_VERSION_MINOR_HIGH) \
	L"." _XWSTR(TRDK_VERSION_MINOR_LOW)

#if defined(_DEBUG)
#	define TRDK_BUILD_IDENTITY \
		"DEBUG." TRDK_VERSION_BRANCH " " TRDK_VERSION_FULL
#	define TRDK_BUILD_IDENTITY_W \
		L"DEBUG." TRDK_VERSION_BRANCH_W L" " TRDK_VERSION_FULL_W
#elif defined(_TEST)
#	define TRDK_BUILD_IDENTITY \
		"TEST." TRDK_VERSION_BRANCH " " TRDK_VERSION_FULL
#	define TRDK_BUILD_IDENTITY_W \
		L"TEST." TRDK_VERSION_BRANCH_W L" " TRDK_VERSION_FULL_W
#elif defined(NTEST) && defined(NDEBUG)
#	define TRDK_BUILD_IDENTITY \
		"RELEASE." TRDK_VERSION_BRANCH " " TRDK_VERSION_FULL
#	define TRDK_BUILD_IDENTITY_W \
		L"RELEASE." TRDK_VERSION_BRANCH_W L" " TRDK_VERSION_FULL_W
#endif

#ifdef DEV_VER
#	define TRDK_BUILD_IDENTITY_ADD		" [" TRDK_BUILD_IDENTITY "]"
#	define TRDK_BUILD_IDENTITY_ADD_W	L" [" TRDK_BUILD_IDENTITY_W L"]"
#else
#	define TRDK_BUILD_IDENTITY_ADD		""
#	define TRDK_BUILD_IDENTITY_ADD_W	L""
#endif

#if defined(_DEBUG)
#	define TRDK_FILE_MODIFICATOR "_dbg"
#elif defined(_TEST)
#	define TRDK_FILE_MODIFICATOR "_test"
#else
#	define TRDK_FILE_MODIFICATOR
#endif

////////////////////////////////////////////////////////////////////////////////

#define TRDK_CORE_FILE_NAME "Core"
#define TRDK_CORE_DLL_FILE_NAME \
	TRDK_CORE_FILE_NAME TRDK_FILE_MODIFICATOR ".dll"

#define TRDK_SERVICES_FILE_NAME "Services"
#define TRDK_SERVICES_DLL_FILE_NAME \
	TRDK_SERVICES_FILE_NAME TRDK_FILE_MODIFICATOR ".dll"

#define TRDK_PYAPI_FILE_NAME "PyApi"
#define TRDK_PYAPI_DLL_FILE_NAME \
	TRDK_PYAPI_FILE_NAME TRDK_FILE_MODIFICATOR ".dll"

#define TRDK_MQLAPI_FILE_NAME "MqlApi"
#define TRDK_MQLAPI_DLL_FILE_NAME \
	TRDK_MQLAPI_FILE_NAME TRDK_FILE_MODIFICATOR ".dll"

#define TRDK_SIMPLEAPI_FILE_NAME "SimpleApi"
#define TRDK_SIMPLEAPI_DLL_FILE_NAME \
	TRDK_SIMPLEAPI_FILE_NAME TRDK_FILE_MODIFICATOR ".dll"

#define TRDK_ENGINE_FILE_NAME "Engine"
#define TRDK_ENGINE_DLL_FILE_NAME \
	TRDK_ENGINE_FILE_NAME TRDK_FILE_MODIFICATOR ".dll"

#define TRDK_ENGINE_SERVER_FILE_NAME "RobotEngine"
#define TRDK_ENGINE_SERVER_EXE_FILE_NAME \
	TRDK_ENGINE_SERVER_FILE_NAME TRDK_FILE_MODIFICATOR ".exe"

#define TRDK_TESTS_FILE_NAME "Tests"
#define TRDK_TESTS_EXE_FILE_NAME \
	TRDK_TESTS_FILE_NAME TRDK_FILE_MODIFICATOR ".exe"

#define TRDK_INTERACTION_FAKE_FILE_NAME "Fake"
#define TRDK_INTERACTION_FAKE_DLL_FILE_NAME \
	TRDK_INTERACTION_FAKE_FILE_NAME TRDK_FILE_MODIFICATOR ".dll"

#define TRDK_INTERACTION_INTERACTIVEBROKERS_FILE_NAME "InteractiveBrokers"
#define TRDK_INTERACTION_INTERACTIVEBROKERS_DLL_FILE_NAME \
	TRDK_INTERACTION_INTERACTIVEBROKERS_FILE_NAME TRDK_FILE_MODIFICATOR ".dll"

#define TRDK_INTERACTION_CSV_FILE_NAME "Csv"
#define TRDK_INTERACTION_CSV_DLL_FILE_NAME \
	TRDK_INTERACTION_CSV_FILE_NAME TRDK_FILE_MODIFICATOR ".dll"

#define TRDK_INTERACTION_LOGREPLAY_FILE_NAME "LogReplay"
#define TRDK_INTERACTION_LOGREPLAY_DLL_FILE_NAME \
	TRDK_INTERACTION_LOGREPLAY_FILE_NAME TRDK_FILE_MODIFICATOR ".dll"

#define TRDK_INTERACTION_ONIXSFIXCONNECTOR_FILE_NAME "OnixsFixConnector"
#define TRDK_INTERACTION_ONIXSFIXCONNECTOR_DLL_FILE_NAME \
	TRDK_INTERACTION_ONIXSFIXCONNECTOR_FILE_NAME TRDK_FILE_MODIFICATOR ".dll"

#define TRDK_INTERACTION_ONIXSHOTSPOT_FILE_NAME "OnixsHotspot"
#define TRDK_INTERACTION_ONIXSHOTSPOT_DLL_FILE_NAME \
	TRDK_INTERACTION_ONIXSHOTSPOT_FILE_NAME TRDK_FILE_MODIFICATOR ".dll"

#define TRDK_INTERACTION_ITCH_FILE_NAME "Itch"
#define TRDK_INTERACTION_ITCH_DLL_FILE_NAME \
	TRDK_INTERACTION_ITCH_FILE_NAME TRDK_FILE_MODIFICATOR ".dll"

#define TRDK_STRATEGY_TEST_FILE_NAME "TestStrategy"
#define TRDK_STRATEGY_TEST_DLL_FILE_NAME \
	TRDK_STRATEGY_TEST_FILE_NAME TRDK_FILE_MODIFICATOR ".dll"

#define TRDK_STRATEGY_FXMB_FILE_NAME "FxMb"
#define TRDK_STRATEGY_FXMB_DLL_FILE_NAME \
	TRDK_STRATEGY_FXMB_FILE_NAME TRDK_FILE_MODIFICATOR ".dll"

////////////////////////////////////////////////////////////////////////////////
