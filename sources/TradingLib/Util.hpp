/*******************************************************************************
 *   Created: 2017/12/17 22:27:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

namespace trdk {
namespace TradingLib {

trdk::Price ReversePrice(const trdk::Price &, const trdk::Security &);
trdk::Qty ReverseQty(const trdk::Price &,
                     const trdk::Qty &,
                     const trdk::Security &);
}
}
