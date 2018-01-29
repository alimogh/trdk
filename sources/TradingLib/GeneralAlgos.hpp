/*******************************************************************************
 *   Created: 2018/01/30 01:46:14
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace TradingLib {

//! Check the position unopened/unclosed rest to minimum order requirements for
//! the current trading system as if this rest will be sent as a separated
//! order.
/** @sa trdk::TradingSystem::CheckOrder
 *  @return True, if trading system may accept order, false otherwise.
 */
bool CheckPositionRestAsOrder(const trdk::Position &);

//! Check the position unopened/unclosed rest to minimum order requirements for
//! the passed trading system as if this rest will be sent as a separated order.
/** @sa trdk::TradingSystem::CheckOrder
 *  @return True, if trading system may accept order, false otherwise.
 */
bool CheckPositionRestAsOrder(const trdk::Position &,
                              const trdk::Security &,
                              const trdk::TradingSystem &);

}  // namespace TradingLib
}  // namespace trdk
