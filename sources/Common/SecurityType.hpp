/**************************************************************************
 *   Created: 2016/04/06 21:02:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk {
namespace Lib {

//! Security types.
BETTER_ENUM(SecurityType,
            std::uint8_t,
            //! Stock.
            Stock,
            //! Future Contract.
            Futures,
            //! Future Option Contract.
            FuturesOptions,
            //! Foreign Exchange Contract.
            For,
            //! Future Option Contract for Foreign Exchange Contract.
            ForFuturesOptions,
            //! Option Contract.
            Options,
            //! Index.
            Index,
            //! Cryptocurrency.
            Crypto);

};  // namespace Lib
}  // namespace trdk
