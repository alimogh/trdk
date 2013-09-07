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

#define _STR(a) #a
#define _XSTR(a) _STR(a)
#define _WSTR(a) L#a
#define _XWSTR(a) _WSTR(a)

#define TRDK_VERSION_MAJOR_HIGH	1
#define TRDK_VERSION_MAJOR_LOW	0
#define TRDK_VERSION_MINOR_HIGH	0
#define TRDK_VERSION_MINOR_LOW	0

#define TRDK_VERSION_BRANCH "Antoine"
#define TRDK_VERSION_BRANCH_W _XWSTR(TRDK_VERSION_BRANCH)

#define TRDK_VENDOR "Eugene V. Palchukovsky"
#define TRDK_VENDOR_W _XWSTR(TRDK_VENDOR)

#define TRDK_DOMAIN "robotdk.com"
#define TRDK_DOMAIN_W _XWSTR(TRDK_DOMAIN)

#define TRDK_LICENSE_SERVICE_SUBDOMAIN "licensing"
#define TRDK_LICENSE_SERVICE_SUBDOMAIN_W _XWSTR(TRDK_LICENSE_SERVICE_SUBDOMAIN)

#define TRDK_SUPPORT_EMAIL_BOX "support"
#define TRDK_SUPPORT_EMAIL_BOX_W _XWSTR(TRDK_SUPPORT_EMAIL_BOX)

#define TRDK_SUPPORT_EMAIL TRDK_SUPPORT_EMAIL_BOX "@" TRDK_DOMAIN
#define TRDK_SUPPORT_EMAIL_W _XWSTR(TRDK_SUPPORT_EMAIL_W)

#define TRDK_NAME "Trading Robot Development Kit"
#define TRDK_NAME_W	_XWSTR(TRDK_NAME)

#define TRDK_COPYRIGHT \
	"Copyright 2013 (C) Eugene V. Palchukovsky. All rights reserved."
#define TRDK_COPYRIGHT_W _XWSTR(TRDK_COPYRIGHT)

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

#ifdef _DEBUG
#	define TRDK_FILE_MODIFICATOR		"_dbg"
#else
#	ifdef _TEST
#		define TRDK_FILE_MODIFICATOR	"_test"
#	else
#		define TRDK_FILE_MODIFICATOR
#	endif
#endif
