/*******************************************************************************
 *   Created: 2017/10/16 00:03:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Api.h"

namespace trdk {
namespace FrontEnd {
namespace Lib {
TRDK_FRONTEND_LIB_API void ShowAbout(QWidget &);
TRDK_FRONTEND_LIB_API void PinToTop(QWidget &, bool pin);
TRDK_FRONTEND_LIB_API QString
ConvertTimeToText(const boost::posix_time::time_duration &);
TRDK_FRONTEND_LIB_API QString ConvertPriceToText(const trdk::Price &,
                                                 uint8_t precision);
TRDK_FRONTEND_LIB_API QString
ConvertPriceToText(const boost::optional<trdk::Price> &, uint8_t precision);
TRDK_FRONTEND_LIB_API QString ConvertQtyToText(const trdk::Qty &,
                                               uint8_t precision);

TRDK_FRONTEND_LIB_API QDateTime
ConvertToQDateTime(const boost::posix_time::ptime &);

class SignalsScopedBlocker {
 public:
  explicit SignalsScopedBlocker(QObject &obj) : m_obj(&obj) {
    Verify(!m_obj->blockSignals(true));
  }

  SignalsScopedBlocker(SignalsScopedBlocker &&) = default;

  ~SignalsScopedBlocker() noexcept { Reset(); }

 private:
  SignalsScopedBlocker(const SignalsScopedBlocker &);
  const SignalsScopedBlocker &operator=(const SignalsScopedBlocker &);

 public:
  void Reset() noexcept {
    if (!m_obj) {
      return;
    }
    try {
      Verify(m_obj->blockSignals(false));
    } catch (...) {
      AssertFailNoException();
      terminate();
    }
    m_obj = nullptr;
  }

 private:
  QObject *m_obj;
};
}
}
}
