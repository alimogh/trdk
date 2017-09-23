/*******************************************************************************
 *   Created: 2017/09/09 15:23:00
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "EngineWindow.hpp"

using namespace trdk::Lib;
using namespace trdk::Frontend::Shell;

namespace {
QString BuildEngineName(const boost::filesystem::path &configFileSubPath) {
  std::vector<std::string> result;
  for (const auto &part : configFileSubPath.branch_path()) {
    result.emplace_back(part.string());
  }
  if (boost::iequals(configFileSubPath.filename().extension().string(),
                     ".ini")) {
    result.emplace_back(configFileSubPath.filename().stem().string());
  } else {
    result.emplace_back(configFileSubPath.filename().string());
  }
  return QString::fromStdString(boost::join(result, " / "));
}
}

EngineWindow::EngineWindow(const boost::filesystem::path &configsBase,
                           const boost::filesystem::path &configFileSubPath,
                           QWidget *parent)
    : QMainWindow(parent),
      m_engine(configsBase / configFileSubPath, parent),
      m_name(BuildEngineName(configFileSubPath)) {
  ui.setupUi(this);
  setWindowTitle(m_name + " - " + QCoreApplication::applicationName());

  connect(ui.actionPinToTop, &QAction::triggered, this,
          &EngineWindow::PinToTop);

  connect(ui.actionStartEngine, &QAction::triggered, this,
          &EngineWindow::Start);
  connect(ui.actionStopEngine, &QAction::triggered, this, &EngineWindow::Stop);
}

void EngineWindow::PinToTop(bool pin) {
  auto flags = windowFlags();
  pin ? flags |= Qt::WindowStaysOnTopHint : flags &= ~Qt::WindowStaysOnTopHint;
  setWindowFlags(flags);
  show();
}

void EngineWindow::Start() {
  for (;;) {
    try {
      m_engine.Start();
    } catch (const std::exception &ex) {
      if (QMessageBox::critical(
              this, tr("Failed to start engine"), QString("%1.").arg(ex.what()),
              QMessageBox::Abort | QMessageBox::Retry) != QMessageBox::Retry) {
        break;
      }
    }
  }
}

void EngineWindow::Stop() {
  for (;;) {
    try {
      m_engine.Stop();
    } catch (const std::exception &ex) {
      if (QMessageBox::critical(
              this, tr("Failed to stop engine"), QString("%1.").arg(ex.what()),
              QMessageBox::Abort | QMessageBox::Retry) != QMessageBox::Retry) {
        break;
      }
    }
  }
}
