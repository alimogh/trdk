/*******************************************************************************
 *   Created: 2017/08/28 10:36:19
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

class Trend : private boost::noncopyable {
 public:
  Trend();
  ~Trend();

 public:
  //! Is trend registered or not.
  /** @sa IsRising
    * @sa IsFalling
    */
  bool IsExistent() const;
  //! Is trend registered and rising.
  /** @sa IsExistent
    * @sa IsFalling
    */
  bool IsRising() const;
  //! Is trend registered and falling.
  /** @sa IsExistent
    * @sa IsFalling
    */
  bool IsFalling() const;

 protected:
  //! Sets "is rising" direction.
  /** @return true if direction is changed by this call, false otherwise.
    */
  bool SetIsRising(bool isRising);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}