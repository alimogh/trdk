/*******************************************************************************
 *   Created: 2017/09/10 00:41:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Engine.hpp"
#include "Engine/Engine.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Frontend::Shell;

namespace sh = trdk::Frontend::Shell;
namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

class sh::Engine::Implementation : private boost::noncopyable {
 public:
  const fs::path m_configFilePath;

  std::unique_ptr<trdk::Engine::Engine> m_engine;

 public:
  explicit Implementation(const fs::path &path) : m_configFilePath(path) {
    // Just a smoke-check that config is an engine config:
    IniFile(m_configFilePath).ReadBoolKey("General", "is_replay_mode");
  }
};

sh::Engine::Engine(const fs::path &path, QWidget *parent)
    : QObject(parent), m_pimpl(boost::make_unique<Implementation>(path)) {}

sh::Engine::~Engine() {}

const fs::path &sh::Engine::GetConfigFilePath() const {
  return m_pimpl->m_configFilePath;
}

void sh::Engine::Start() {
  if (m_pimpl->m_engine) {
    throw Exception(tr("Engine already started").toLocal8Bit().constData());
  }
  m_pimpl->m_engine = boost::make_unique<trdk::Engine::Engine>(
      GetConfigFilePath(),
      [&](const Context::State & /*newState*/, const std::string *) {},
      [](trdk::Engine::Context::Log & /*log*/) {},
      boost::unordered_map<std::string, std::string>());
}

void sh::Engine::Stop() {
  if (!m_pimpl->m_engine) {
    throw Exception(tr("Engine is not started").toLocal8Bit().constData());
  }
  m_pimpl->m_engine->Stop(STOP_MODE_IMMEDIATELY);
  m_pimpl->m_engine.reset();
}
