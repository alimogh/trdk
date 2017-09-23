/*******************************************************************************
 *   Created: 2017/09/21 20:16:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#ifdef BOOST_WINDOWS
#define TRDK_INTERACTION_FIXPROTOCOL_API
#else
#define TRDK_INTERACTION_FIXPROTOCOL_API extern "C"
#endif
