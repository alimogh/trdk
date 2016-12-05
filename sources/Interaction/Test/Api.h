/**************************************************************************
 *   Created: 2016/02/08 18:58:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#ifdef BOOST_WINDOWS
#	define TRDK_INTERACTION_TEST_API
#else
#	define TRDK_INTERACTION_TEST_API extern "C"
#endif
