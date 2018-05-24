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
using namespace trdk::FrontEnd;
using namespace Terminal;

namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////////////

namespace {

class SplashScreen : public QSplashScreen {
 public:
  SplashScreen() : QSplashScreen(QPixmap(":/Terminal/Resources/splash.png")) {}
  SplashScreen(SplashScreen &&) = default;
  SplashScreen(const SplashScreen &) = delete;
  SplashScreen &operator=(SplashScreen &&) = delete;
  SplashScreen &operator=(const SplashScreen &) = delete;
  ~SplashScreen() = default;

  void ShowMessage(const std::string &message) {
    showMessage(QString::fromStdString(message),
                Qt::AlignRight | Qt::AlignBottom, QColor(240, 240, 240));
  }
};

fs::path GetLogsDir() {
  fs::path result;
#ifndef _DEBUG
  {
    const auto &locations =
        QStandardPaths::standardLocations(QStandardPaths::DataLocation);
    AssertLt(0, locations.size());
    if (!locations.isEmpty()) {
      result = locations.front().toStdString();
    }
  }
#endif
  result /= "Logs";
  return result;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
  _CrtSetDbgFlag(0);

#ifdef DEV_VER
  QStandardPaths::setTestModeEnabled(true);
#endif

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
        GetStandardFilePath("default.ini", QStandardPaths::AppDataLocation),
        GetLogsDir(), nullptr);
    std::vector<std::unique_ptr<Dll>> moduleDlls;

    for (;;) {
      try {
        engine.Start([&splash](const std::string &message) {
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

    MainWindow mainWindow(engine, moduleDlls, nullptr);
    mainWindow.setWindowTitle(application.applicationName()
#ifdef DEV_VER
                              + " (build: " TRDK_BUILD_IDENTITY
                                ", build time: " __DATE__ " " __TIME__ ")"
#endif
    );
    mainWindow.setEnabled(false);
    mainWindow.setWindowState(Qt::WindowMaximized);
    mainWindow.show();
    splash->setWindowModality(Qt::ApplicationModal);
    splash->activateWindow();

    {
      splash->ShowMessage(
          application.tr("Loading strategy modules...").toStdString());
      mainWindow.LoadModule("ArbitrationAdvisor");
      mainWindow.LoadModule("TriangularArbitrage");
      mainWindow.LoadModule("MarketMaker");
      mainWindow.LoadModule("PingPong");
    }

    {
      splash->ShowMessage(
          application.tr("Restoring module instances...").toStdString());
      mainWindow.RestoreModules();
    }

    mainWindow.setEnabled(true);
    splash->finish(&mainWindow);
    splash.reset();

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
