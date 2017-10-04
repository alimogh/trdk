/*******************************************************************************
 *   Created: 2017/10/01 19:18:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#ifdef BOOST_WINDOWS
#ifdef TRDK_FRONTEND_SHELL_LIB
#define TRDK_FRONTEND_SHELL_LIB_API Q_DECL_EXPORT
#else
#define TRDK_FRONTEND_SHELL_LIB_API Q_DECL_IMPORT
#endif
#else
#define TRDK_FRONTEND_SHELL_LIB_API
#endif