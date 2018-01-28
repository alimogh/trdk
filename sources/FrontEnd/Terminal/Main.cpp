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

////////////////////////////////////////////////////////////////////////////////

namespace {
class SplashScreen : public QSplashScreen {
 public:
  SplashScreen() : QSplashScreen(QPixmap(":/Terminal/Resources/splash.png")) {}
  virtual ~SplashScreen() override = default;

 public:
  void ShowMessage(const std::string &message) {
    showMessage(QString::fromStdString(message),
                Qt::AlignRight | Qt::AlignBottom, QColor(240, 240, 240));
  }

 protected:
  virtual void mousePressEvent(QMouseEvent *) override{};
};
}  // namespace

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
  _CrtSetDbgFlag(0);

  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication application(argc, argv);

  auto splash = boost::make_unique<SplashScreen>();
  splash->show();

  try {
    application.setApplicationName(TRDK_NAME);
    application.setOrganizationDomain(TRDK_DOMAIN);
    splash->ShowMessage(
        application.tr("Loading " TRDK_NAME "...").toStdString());

    LoadStyle(application);

    Engine engine(
        GetExeFilePath().branch_path() / "etc" / "Robot" / "default.ini",
        nullptr);
    std::vector<std::unique_ptr<trdk::Lib::Dll>> moduleDlls;

    MainWindow mainWindow(engine, moduleDlls, nullptr);

    for (;;) {
      try {
        mainWindow.GetEngine().Start([&splash](const std::string &message) {
          splash->ShowMessage(message);
        });
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
    {
      // Custom branch logic: starts first 3 strategies by default.
      mainWindow.CreateNewArbitrageStrategy();
      mainWindow.CreateNewArbitrageStrategy();
    }

    mainWindow.show();
    splash->finish(&mainWindow);
    splash.reset();

    engine.Test();

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

////////////////////////////////////////////////////////////////////////////////
