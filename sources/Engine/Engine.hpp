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

class TRDK_ENGINE_API Engine : private boost::noncopyable {
 public:
  explicit Engine(
      const boost::filesystem::path &,
      const trdk::Engine::Context::StateUpdateSlot &contextStateUpdateSlot,
      const boost::function<void(const std::string &)> &startProgressCallback,
      const boost::function<bool(const std::string &)> &startErrorCallback,
      const boost::function<void(trdk::Engine::Context::Log &)>
          &logStartCallback,
      const boost::unordered_map<std::string, std::string> &params);
  explicit Engine(
      const boost::filesystem::path &,
      const trdk::Engine::Context::StateUpdateSlot &contextStateUpdateSlot,
      DropCopy &dropCopy,
      const boost::function<void(const std::string &)> &startProgressCallback,
      const boost::function<bool(const std::string &)> &startErrorCallback,
      const boost::function<void(trdk::Engine::Context::Log &)>
          &logStartCallback,
      const boost::unordered_map<std::string, std::string> &params);
  ~Engine();

 public:
  trdk::Context &GetContext();

  void Stop(const trdk::StopMode &);

  void ClosePositions();

 private:
  void VerifyModules() const;

 private:
  void Run(
      const boost::filesystem::path &,
      const trdk::Context::StateUpdateSlot &,
      DropCopy *dropCopy,
      const boost::function<void(const std::string &)> &startProgressCallback,
      const boost::function<bool(const std::string &)> &startErrorCallback,
      const boost::function<void(trdk::Engine::Context::Log &)>
          &logStartCallback,
      const boost::unordered_map<std::string, std::string> &params);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
