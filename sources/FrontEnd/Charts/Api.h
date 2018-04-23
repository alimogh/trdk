//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#ifdef BOOST_WINDOWS
#ifdef TRDK_FRONTEND_CHARTS
#define TRDK_FRONTEND_CHARTS_API Q_DECL_EXPORT
#else
#define TRDK_FRONTEND_CHARTS_API Q_DECL_IMPORT
#endif
#else
#define TRDK_FRONTEND_CHARTS_API
#endif