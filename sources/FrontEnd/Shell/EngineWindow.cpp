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
#include "Core/Security.hpp"
#include "Lib/SecurityListModel.hpp"
#include "OrderWindow.hpp"
#include "ShellLib/Module.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd::Lib;
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

  m_ui.securityList->setModel(new SecurityListModel(m_engine, this));
  m_ui.securityList->verticalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);

  setWindowTitle(m_name + " - " + QCoreApplication::applicationName());

  LoadModule();

  Verify(connect(m_ui.pinToTop, &QAction::triggered,
                 [this](bool pin) { PinToTop(*this, pin); }));

  Verify(connect(m_ui.startEngine, &QAction::triggered, this,
                 &EngineWindow::Start));
  Verify(
      connect(m_ui.stopEngine, &QAction::triggered, this, &EngineWindow::Stop));

  Verify(connect(&m_engine, &Lib::Engine::StateChanged, this,
                 &EngineWindow::OnStateChanged, Qt::QueuedConnection));
  Verify(connect(&m_engine, &Lib::Engine::Message, this,
                 &EngineWindow::OnMessage, Qt::QueuedConnection));
  Verify(connect(&m_engine, &Lib::Engine::LogRecord, this,
                 &EngineWindow::OnLogRecord, Qt::QueuedConnection));

  Verify(connect(m_ui.securityList, &QTableView::doubleClicked,
                 [this](const QModelIndex &index) {
                   ShowOrderWindow(
                       boost::polymorphic_downcast<SecurityListModel *>(
                           m_ui.securityList->model())
                           ->GetSecurity(index));
                 }));
}

void EngineWindow::LoadModule() {
  Assert(!m_moduleDll);
  AssertEq(0, m_modules.size());

  const IniFile conf(m_engine.GetConfigFilePath());
  const IniSectionRef frontEndConf(conf, "Front-end");
  if (!frontEndConf) {
    return;
  }

  const std::string &key = "module";
  if (!frontEndConf.IsKeyExist(key)) {
    return;
  }

  ModuleFactoryResult modules;
  {
    const auto &file = frontEndConf.ReadFileSystemPath(key);
    try {
      m_moduleDll = std::make_unique<Dll>(file, true);
      modules = m_moduleDll->GetFunction<ModuleFactory>(GetModuleFactoryName())(
          m_engine, this);
    } catch (const std::exception &ex) {
      const auto &error =
          QString("Failed to load engine front-end module \"%1\": \"%2\".")
              .arg(QString::fromStdString(file.string()), ex.what());
      QMessageBox::critical(this, tr("Failed to load engine front-end."), error,
                            QMessageBox::Ignore);
      throw;
    }
  }

  for (auto &module : boost::adaptors::reverse(modules)) {
    Assert(!module.first.isEmpty());
    Assert(module.second);
    m_modules.emplace_back(std::move(module.second));
    const auto &moduleWidget = *m_modules.back();
    if (minimumWidth() < moduleWidget.minimumWidth()) {
      setMinimumWidth(moduleWidget.minimumWidth());
    }
    if (minimumHeight() < moduleWidget.minimumHeight()) {
      setMinimumHeight(moduleWidget.minimumHeight());
    }
    m_ui.content->insertTab(0, &*m_modules.back(), std::move(module.first));
  }
  m_ui.content->setCurrentIndex(0);

  adjustSize();
}

void EngineWindow::Start(bool start) {
  Assert(start);
  if (start) {
    for (;;) {
      try {
        m_engine.Start([](const std::string &) {});
        break;
      } catch (const std::exception &ex) {
        if (QMessageBox::critical(this, tr("Failed to start engine"),
                                  QString("%1.").arg(ex.what()),
                                  QMessageBox::Abort | QMessageBox::Retry) !=
            QMessageBox::Retry) {
          OnStateChanged(false);
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
          OnStateChanged(true);
          break;
        }
      }
    }
  }
}

void EngineWindow::OnStateChanged(bool isStarted) {
  m_ui.startEngine->setEnabled(!isStarted);
  m_ui.startEngine->setChecked(isStarted);
  m_ui.stopEngine->setEnabled(isStarted);
  m_ui.stopEngine->setChecked(!isStarted);
  m_ui.securityList->setEnabled(isStarted);
}

void EngineWindow::OnMessage(const QString &message, bool isWarning) {
  if (isWarning) {
    QMessageBox::warning(this, tr("Engine warning"), message, QMessageBox::Ok);
  } else {
    QMessageBox::information(this, tr("Engine information"), message,
                             QMessageBox::Ok);
  }
}

void EngineWindow::OnLogRecord(const QString &message) {
  m_ui.log->moveCursor(QTextCursor::End);
  m_ui.log->insertPlainText(message + "\n");
  m_ui.log->moveCursor(QTextCursor::End);
}

void EngineWindow::ShowOrderWindow(Security &security) {
  const auto &key = security.GetSymbol();
  {
    const auto &it = m_orderWindows.find(key);
    if (it != m_orderWindows.cend()) {
      it->second->activateWindow();
      it->second->showNormal();
      it->second->SetSecurity(security);
      return;
    }
  }
  {
    auto &window =
        *m_orderWindows
             .emplace(key, boost::make_unique<OrderWindow>(m_engine, this))
             .first->second;
    connect(&window, &OrderWindow::Closed,
            [this, key]() { CloseOrderWindow(key); });
    window.show();
    window.SetSecurity(security);
  }
}

void EngineWindow::CloseOrderWindow(const Symbol &symbol) {
  Assert(m_orderWindows.find(symbol) != m_orderWindows.cend());
  m_orderWindows.erase(symbol);
}
