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

#if defined(_MSC_VER)
#ifdef TRDK_INTERACTION_REST
#define TRDK_INTERACTION_REST_API __declspec(dllexport)
#else
#define TRDK_INTERACTION_REST_API __declspec(dllimport)
#endif
#endif
