/*******************************************************************************
 *   Created: 2017/09/09 02:24:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Engine.hpp"

using namespace trdk::Lib;
using namespace trdk::Frontend::Shell;

namespace fs = boost::filesystem;

Engine::Engine(const fs::path &path) : m_path(path) {
  IniFile(m_path).ReadBoolKey("General", "is_replay_mode");
}

Engine::Engine(Engine &&rhs) : m_path(std::move(rhs.m_path)) {}

const fs::path &Engine::GetConfigurationFilePath() const { return m_path; }