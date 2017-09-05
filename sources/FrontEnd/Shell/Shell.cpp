/*******************************************************************************
 *   Created: 2017/09/05 08:18:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Shell.hpp"

Shell::Shell(QWidget *parent) : QMainWindow(parent) {
  ui.setupUi(this);
  setWindowTitle(QCoreApplication::applicationName() + " " +
                 TRDK_BUILD_IDENTITY);

  connect(ui.actionAbout, &QAction::triggered, this, &Shell::ShowAboutInfo);
}

void Shell::ShowAboutInfo() {
  const auto &text = tr("%1\nVersion %2 (%3, x%4-bit)\n\nVendor: %5\nWebsite: "
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
  QMessageBox::about(this, tr("About"), text);
}
