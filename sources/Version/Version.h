
#pragma once

#define TRDK_VERSION_RELEASE	2
#define TRDK_VERSION_BUILD	2
#define TRDK_VERSION_STATUS	0

#define TRDK_VERSION_BRANCH		"master"
#define TRDK_VERSION_BRANCH_W	L"master"

#define TRDK_VENDOR		"MBCMTRADING"
#define TRDK_VENDOR_W	L"MBCMTRADING"

#define TRDK_DOMAIN		"mb-cm.com"
#define TRDK_DOMAIN_W	L"mb-cm.com"

#define TRDK_SUPPORT_EMAIL		"eugene@mb-cm.com"
#define TRDK_SUPPORT_EMAIL_W	L"eugene@mb-cm.com"

#define TRDK_LICENSE_SERVICE_SUBDOMAIN		"licensing"
#define TRDK_LICENSE_SERVICE_SUBDOMAIN_W	L"licensing"

#define TRDK_NAME	"MBCMTRADING Trading Robot Development Kit"
#define TRDK_NAME_W	L"MBCMTRADING Trading Robot Development Kit"

#define TRDK_COPYRIGHT		"Copyright 2016 (C) MBCMTRADING, mb-cm.com. All rights reserved."
#define TRDK_COPYRIGHT_W	L"Copyright 2016 (C) MBCMTRADING, mb-cm.com. All rights reserved."

#define TRDK_CONCURRENCY_PROFILE_DEBUG (::trdk::Lib::Concurrency::PROFILE_RELAX)
#define TRDK_CONCURRENCY_PROFILE_TEST (::trdk::Lib::Concurrency::PROFILE_HFT)
#define TRDK_CONCURRENCY_PROFILE_RELEASE (::trdk::Lib::Concurrency::PROFILE_HFT)

#define TRDK_GET_REQUIRED_MODUE_FILE_NAME_LIST() {"Core", "Engine", "Fake", "OnixsFixConnector", "Itch", "TestStrategy", "FxMb", };

