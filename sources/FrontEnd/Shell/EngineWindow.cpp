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
using namespace trdk::FrontEnd::Shell;

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
  m_ui.setupUi(this);
  setWindowTitle(m_name + " - " + QCoreApplication::applicationName());

  LoadModule();

  connect(m_ui.pinToTop, &QAction::triggered, this, &EngineWindow::PinToTop);

  connect(m_ui.startEngine, &QAction::triggered, this, &EngineWindow::Start);
  connect(m_ui.stopEngine, &QAction::triggered, this, &EngineWindow::Stop);

  connect(&m_engine, &Engine::StateChanged, this, &EngineWindow::StateChanged,
          Qt::QueuedConnection);
  connect(&m_engine, &Engine::Message, this, &EngineWindow::Message,
          Qt::QueuedConnection);
  connect(&m_engine, &Engine::LogRecord, this, &EngineWindow::LogRecord,
          Qt::QueuedConnection);
}

void EngineWindow::LoadModule() {
  Assert(!m_moduleDll);
  Assert(!m_module);

  const IniFile conf(m_engine.GetConfigFilePath());
  const IniSectionRef frontEndConf(conf, "Front-end");
  if (!frontEndConf) {
    return;
  }

  const std::string &key = "module";
  if (!frontEndConf.IsKeyExist(key)) {
    return;
  }

  {
    const auto &file = frontEndConf.ReadFileSystemPath(key);
    try {
      m_moduleDll = std::make_unique<Dll>(file, true);
      const auto &createEngineFrontEnd =
          m_moduleDll->GetFunction<std::unique_ptr<QWidget>(QWidget *)>(
              "CreateEngineFrontEnd");
      m_module = createEngineFrontEnd(this);
    } catch (const std::exception &ex) {
      const auto &error =
          QString("Failed to load engine front-end module \"%1\": \"%2\".")
              .arg(QString::fromStdString(file.string()), ex.what());
      QMessageBox::critical(this, tr("Failed to load engine front-end."), error,
                            QMessageBox::Ignore);
      throw;
    }
  }
  Assert(m_module);

  m_ui.verticalLayout->insertWidget(0, &*m_module);
  if (minimumWidth() < m_module->minimumWidth()) {
    setMinimumWidth(m_module->minimumWidth());
  }
  setMinimumHeight(minimumHeight() + m_module->minimumHeight());

  setFixedSize(width() < m_module->width() ? m_module->width() : width(),
               height() + m_module->height());

  adjustSize();
}

void EngineWindow::PinToTop(bool pin) {
  auto flags = windowFlags();
  pin ? flags |= Qt::WindowStaysOnTopHint : flags &= ~Qt::WindowStaysOnTopHint;
  setWindowFlags(flags);
  show();
}

void EngineWindow::Start(bool start) {
  Assert(start);
  if (start) {
    for (;;) {
      try {
        m_engine.Start();
        break;
      } catch (const std::exception &ex) {
        if (QMessageBox::critical(this, tr("Failed to start engine"),
                                  QString("%1.").arg(ex.what()),
                                  QMessageBox::Abort | QMessageBox::Retry) !=
            QMessageBox::Retry) {
          StateChanged(false);
          break;
        }
      }
    }
  }
}

void EngineWindow::Stop(bool stop) {
  Assert(stop);
  if (stop) {
    for (;;) {
      try {
        m_engine.Stop();
        break;
      } catch (const std::exception &ex) {
        if (QMessageBox::critical(this, tr("Failed to stop engine"),
                                  QString("%1.").arg(ex.what()),
                                  QMessageBox::Abort | QMessageBox::Retry) !=
            QMessageBox::Retry) {
          StateChanged(true);
          break;
        }
      }
    }
  }
}

void EngineWindow::StateChanged(bool isStarted) {
  m_ui.startEngine->setEnabled(!isStarted);
  m_ui.startEngine->setChecked(isStarted);
  m_ui.stopEngine->setEnabled(isStarted);
  m_ui.stopEngine->setChecked(!isStarted);
}

void EngineWindow::Message(const QString &message, bool isWarning) {
  if (isWarning) {
    QMessageBox::warning(this, tr("Engine warning"), message, QMessageBox::Ok);
  } else {
    QMessageBox::information(this, tr("Engine information"), message,
                             QMessageBox::Ok);
  }
}

void EngineWindow::LogRecord(const QString &message) {
  m_ui.log->moveCursor(QTextCursor::End);
  m_ui.log->insertPlainText(message + "\n");
  m_ui.log->moveCursor(QTextCursor::End);
}
