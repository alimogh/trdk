/**************************************************************************
 *   Created: 2016/10/30 17:00:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#ifdef TRDK_INTERACTION_TRANSAQ
#ifdef BOOST_WINDOWS
#define TRDK_INTERACTION_TRANSAQ_API __declspec(dllexport)
#else
#define TRDK_INTERACTION_TRANSAQ_API
#endif
#else
#ifdef BOOST_WINDOWS
#define TRDK_INTERACTION_TRANSAQ_API __declspec(dllimport)
#else
#define TRDK_INTERACTION_TRANSAQ_API
#endif
#endif
