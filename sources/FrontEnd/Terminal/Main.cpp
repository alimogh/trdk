/*******************************************************************************
 *   Created: 2017/10/15 21:07:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Lib/Engine.hpp"
#include "Lib/Style.hpp"
#include "MainWindow.hpp"

using namespace trdk::Lib;
using namespace trdk::FrontEnd::Lib;
using namespace trdk::FrontEnd::Terminal;

int main(int argc, char *argv[]) {
  _CrtSetDbgFlag(0);

  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication application(argc, argv);

  try {
    application.setApplicationName(TRDK_NAME);
    application.setOrganizationDomain(TRDK_DOMAIN);

    LoadStyle(application);

    auto engine = boost::make_unique<Engine>(
        GetExeFilePath().branch_path() / "etc" / "Robot" / "default.ini",
        nullptr);

    MainWindow mainWindow(std::move(engine), nullptr);
    mainWindow.show();

    for (;;) {
      try {
        mainWindow.GetEngine().Start();
        break;
      } catch (const std::exception &ex) {
        if (QMessageBox::critical(nullptr, application.tr("Failed to start"),
                                  QString("%1.").arg(ex.what()),
                                  QMessageBox::Abort | QMessageBox::Retry) !=
            QMessageBox::Retry) {
          return 1;
        }
      }
    }

    mainWindow.CreateNewArbitrageStrategy();

    return application.exec();
  } catch (const std::exception &ex) {
    QMessageBox::critical(nullptr, application.tr("Application fatal error"),
                          QString("%1.").arg(ex.what()), QMessageBox::Abort);
  } catch (...) {
    AssertFailNoException();
    QMessageBox::critical(nullptr, application.tr("Application fatal error"),
                          application.tr("Unknown error."), QMessageBox::Abort);
  }
  return 1;
}
