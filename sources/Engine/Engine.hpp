/**************************************************************************
 *   Created: 2013/02/03 11:26:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Api.h"
#include "Context.hpp"

namespace trdk {
namespace Engine {

class TRDK_ENGINE_API Engine {
 public:
  explicit Engine(
      const boost::filesystem::path& configFile,
      const boost::filesystem::path& logsDir,
      const Context::StateUpdateSlot& contextStateUpdateSlot,
      const boost::function<void(const std::string&)>& startProgressCallback,
      const boost::function<bool(const std::string&)>& startErrorCallback,
      const boost::function<void(Context::Log&)>& logStartCallback,
      const boost::unordered_map<std::string, std::string>& params);
  explicit Engine(
      const boost::filesystem::path& configFile,
      const boost::filesystem::path& logsDir,
      const Context::StateUpdateSlot& contextStateUpdateSlot,
      DropCopy& dropCopy,
      const boost::function<void(const std::string&)>& startProgressCallback,
      const boost::function<bool(const std::string&)>& startErrorCallback,
      const boost::function<void(Context::Log&)>& logStartCallback,
      const boost::unordered_map<std::string, std::string>& params);
  Engine(Engine&&);
  Engine(const Engine&) = delete;
  ~Engine();

  Engine& operator=(Engine&&) = delete;
  Engine& operator=(const Engine&) = delete;

  trdk::Context& GetContext();

  void Stop(const StopMode&);

  void ClosePositions();

 private:
  void VerifyModules() const;

  void Run(
      const boost::filesystem::path& configFile,
      const boost::filesystem::path& logsDir,
      const Context::StateUpdateSlot&,
      DropCopy* dropCopy,
      const boost::function<void(const std::string&)>& startProgressCallback,
      const boost::function<bool(const std::string&)>& startErrorCallback,
      const boost::function<void(Context::Log&)>& logStartCallback,
      const boost::unordered_map<std::string, std::string>& params);

  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace Engine
}  // namespace trdk
