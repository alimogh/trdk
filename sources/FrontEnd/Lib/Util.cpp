/*******************************************************************************
 *   Created: 2017/10/16 00:04:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Util.hpp"

using namespace trdk::FrontEnd;

void Lib::ShowAbout(QWidget &parent) {
  const auto &text =
      parent
          .tr("%1\nVersion %2 (%3, x%4-bit)\n\nVendor: %5\nWebsite: "
              "%6\nSupport email: %7")
          .arg(TRDK_NAME,            // 1
               TRDK_BUILD_IDENTITY,  // 2
               TRDK_VERSION_BRANCH,  // 3
#if defined(_M_IX86)
               "32"
#elif defined(_M_X64)
               "64"
#endif
               ,                     // 4
               TRDK_VENDOR,          // 5
               TRDK_DOMAIN,          // 6
               TRDK_SUPPORT_EMAIL);  // 7
  QMessageBox::about(&parent, parent.tr("About"), text);
}

void Lib::PinToTop(bool pin, QWidget &widget) {
  auto flags = widget.windowFlags();
  pin ? flags |= Qt::WindowStaysOnTopHint : flags &= ~Qt::WindowStaysOnTopHint;
  widget.setWindowFlags(flags);
  widget.show();
}
