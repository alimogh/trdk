/*******************************************************************************
 *   Created: 2017/09/05 08:19:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Lib/Style.hpp"
#include "MainWindow.hpp"

using namespace trdk::FrontEnd::Lib;
using namespace trdk::FrontEnd::Shell;

int main(int argc, char *argv[]) {
  _CrtSetDbgFlag(0);

  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication application(argc, argv);

  try {
    application.setApplicationName(TRDK_NAME);
    application.setOrganizationDomain(TRDK_DOMAIN);

    LoadStyle(application);

    MainWindow mainWindow(Q_NULLPTR);
    mainWindow.show();

    return application.exec();
  } catch (const std::exception &ex) {
    QMessageBox::critical(nullptr, application.tr("Application fatal error"),
                          QString("%1.").arg(ex.what()), QMessageBox::Abort);
  } catch (...) {
    AssertFailNoException();
    QMessageBox::critical(nullptr, application.tr("Application fatal error"),
                          application.tr("Unknown error."), QMessageBox::Abort);
  }
}
