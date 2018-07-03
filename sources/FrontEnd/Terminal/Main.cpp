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
#include "Application.hpp"
#include "Config.hpp"
#include "Lib/Engine.hpp"
#include "Lib/Style.hpp"
#include "MainWindow.hpp"

using namespace trdk::Lib;
using namespace trdk::FrontEnd;
using namespace Terminal;
namespace fs = boost::filesystem;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

namespace {

class SplashScreen : public QSplashScreen {
 public:
  SplashScreen() : QSplashScreen(QPixmap{":/Terminal/Resources/Splash.png"}) {}
  SplashScreen(SplashScreen &&) = delete;
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
#else
  result = GetExeFilePath().branch_path() / "Var";
#endif
  result /= "Logs";
  fs::create_directories(result);
  return result;
}

fs::path GetConfigFilePath() {
  return GetStandardFilePath("config.json", QStandardPaths::AppDataLocation);
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
  _CrtSetDbgFlag(0);

#ifdef DEV_VER
  QStandardPaths::setTestModeEnabled(true);
#endif

  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  Application application(argc, argv);

  try {
    QApplication::setApplicationName(TRDK_NAME);
    QApplication::setOrganizationDomain(TRDK_DOMAIN);

    CheckConfig(GetConfigFilePath());

    auto splash = boost::make_unique<SplashScreen>();
    splash->show();

    splash->ShowMessage(QObject::tr("Loading " TRDK_NAME "...").toStdString());

    LoadStyle(application);

    Engine engine(GetConfigFilePath(), GetLogsDir(), nullptr);

    std::vector<std::unique_ptr<Dll>> moduleDlls;

    try {
      engine.Start([&splash](const std::string &message) {
        splash->ShowMessage(message);
      });
    } catch (const std::exception &ex) {
      QMessageBox::critical(nullptr, QObject::tr("Failed to start"),
                            QString("%1.").arg(ex.what()), QMessageBox::Abort);
      return 1;
    }

    MainWindow mainWindow(engine, moduleDlls, nullptr);
    mainWindow.setWindowTitle(QApplication::applicationName()
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
          QObject::tr("Loading strategy modules...").toStdString());
      mainWindow.LoadModule("ArbitrationAdvisor");
      mainWindow.LoadModule("TriangularArbitrage");
      mainWindow.LoadModule("MarketMaker");
      mainWindow.LoadModule("PingPong");
    }

    {
      splash->ShowMessage(
          QObject::tr("Restoring module instances...").toStdString());
      mainWindow.RestoreModules();
    }

    mainWindow.setEnabled(true);
    splash->finish(&mainWindow);
    splash.reset();

    return QApplication::exec();
  } catch (const CheckConfigException &) {
    QMessageBox::warning(
        nullptr,
        QObject::tr("%1 configuration required")
            .arg(QApplication::applicationName()),
        QObject::tr(
            "Impossible to continue to use application with the "
            "current configuration. To configure, run the application again."),
        QMessageBox::Close);
  } catch (const std::exception &ex) {
    QMessageBox::critical(
        nullptr,
        QObject::tr("%1 fatal error").arg(QApplication::applicationName()),
        QObject::tr(
            "An unexpected program failure occurred: %1. "
            "\n\nApplication is terminated.\n\nPlease contact support by %2 "
            "with descriptions of actions that you have made during "
            "the occurrence of the failure.")
            .arg(ex.what(), TRDK_SUPPORT_EMAIL),
        QMessageBox::Abort);
  } catch (...) {
    AssertFailNoException();
    QMessageBox::critical(
        nullptr,
        QObject::tr("%1 fatal error").arg(QApplication::applicationName()),
        QObject::tr(
            "An unexpected unknown program failure occurred! "
            "\n\nApplication is terminated.\n\nPlease contact support by %1 "
            "with descriptions of actions that you have made during "
            "the occurrence of the failure.")
            .arg(TRDK_SUPPORT_EMAIL),
        QMessageBox::Abort);
  }
  return 1;
}

////////////////////////////////////////////////////////////////////////////////
