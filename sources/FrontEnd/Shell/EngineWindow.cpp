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
      m_path(configsBase / configFileSubPath),
      m_name(BuildEngineName(configFileSubPath)) {
  // Just a smoke-check that config is an engine config:
  IniFile(m_path).ReadBoolKey("General", "is_replay_mode");
  ui.setupUi(this);
  setWindowTitle(m_name + " - " + QCoreApplication::applicationName());
}
