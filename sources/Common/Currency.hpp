/**************************************************************************
 *   Created: 2014/08/17 16:44:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include <enum.h>

namespace trdk {
namespace Lib {

//! Currency.
BETTER_ENUM(Currency,
            std::uint16_t,
            EUR = 978,
            EURT = 1978,
            USD = 840,
            USDT = 1840,
            GBP = 826,
            RUB = 810,
            BTC = 2001,
            ETH = 2002,
            LTC = 2003,
            XRP = 2004);

}  // namespace Lib
}  // namespace trdk
