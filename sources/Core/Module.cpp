/**************************************************************************
 *   Created: 2012/09/23 14:31:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Module.hpp"
#include "EventsLog.hpp"
#include "ModuleSecurityList.hpp"
#include "Security.hpp"
#include "Settings.hpp"
#include "TradingLog.hpp"

namespace fs = boost::filesystem;
namespace uuids = boost::uuids;
namespace ptr = boost::property_tree;
using namespace trdk;
using namespace Lib;

////////////////////////////////////////////////////////////////////////////////

Module::SecurityList::Iterator::Iterator(
    std::unique_ptr<Implementation> &&pimpl)
    : m_pimpl(std::move(pimpl)) {
  Assert(m_pimpl);
}

Module::SecurityList::Iterator::Iterator(const Iterator &rhs)
    : m_pimpl(boost::make_unique<Implementation>(*rhs.m_pimpl)) {}

Module::SecurityList::Iterator::~Iterator() = default;

Module::SecurityList::Iterator &Module::SecurityList::Iterator::operator=(
    const Iterator &rhs) {
  Assert(this != &rhs);
  Iterator(rhs).Swap(*this);
  return *this;
}

void Module::SecurityList::Iterator::Swap(Iterator &rhs) {
  Assert(this != &rhs);
  std::swap(m_pimpl, rhs.m_pimpl);
}

Security &Module::SecurityList::Iterator::dereference() const {
  return *m_pimpl->iterator->second;
}

bool Module::SecurityList::Iterator::equal(const Iterator &rhs) const {
  return m_pimpl->iterator == rhs.m_pimpl->iterator;
}

bool Module::SecurityList::Iterator::equal(const ConstIterator &rhs) const {
  return m_pimpl->iterator == rhs.m_pimpl->iterator;
}

void Module::SecurityList::Iterator::increment() { ++m_pimpl->iterator; }

Module::SecurityList::ConstIterator::ConstIterator(
    std::unique_ptr<Implementation> &&pimpl)
    : m_pimpl(std::move(pimpl)) {
  Assert(m_pimpl);
}

Module::SecurityList::ConstIterator::ConstIterator(const Iterator &rhs)
    : m_pimpl(boost::make_unique<Implementation>(rhs.m_pimpl->iterator)) {}

Module::SecurityList::ConstIterator::ConstIterator(const ConstIterator &rhs)
    : m_pimpl(boost::make_unique<Implementation>(*rhs.m_pimpl)) {}

Module::SecurityList::ConstIterator::~ConstIterator() = default;

Module::SecurityList::ConstIterator &Module::SecurityList::ConstIterator::
operator=(const ConstIterator &rhs) {
  Assert(this != &rhs);
  ConstIterator(rhs).Swap(*this);
  return *this;
}

void Module::SecurityList::ConstIterator::Swap(ConstIterator &rhs) {
  Assert(this != &rhs);
  std::swap(m_pimpl, rhs.m_pimpl);
}

const Security &Module::SecurityList::ConstIterator::dereference() const {
  return *m_pimpl->iterator->second;
}

bool Module::SecurityList::ConstIterator::equal(
    const ConstIterator &rhs) const {
  Assert(this != &rhs);
  return m_pimpl->iterator == rhs.m_pimpl->iterator;
}

bool Module::SecurityList::ConstIterator::equal(const Iterator &rhs) const {
  return m_pimpl->iterator == rhs.m_pimpl->iterator;
}

void Module::SecurityList::ConstIterator::increment() { ++m_pimpl->iterator; }

//////////////////////////////////////////////////////////////////////////

namespace {

boost::atomic<Module::InstanceId> nextFreeInstanceId(1);

std::string FormatStringId(const std::string &typeName,
                           const std::string &implementationName,
                           const std::string &instanceName,
                           const Module::InstanceId &instanceId) {
  std::ostringstream result;
  result << typeName << '.' << implementationName;
  if (implementationName != instanceName) {
    result << '.' << instanceName;
  }
  result << '.' << instanceId;
  return result.str();
}
}  // namespace

class Module::Implementation : private boost::noncopyable {
 public:
  const uuids::uuid m_id;
  const InstanceId m_instanceId;
  const std::string m_implementationName;
  const std::string m_instanceName;

  Mutex m_mutex;

  Context &m_context;

  const std::string m_typeName;
  const std::string m_tag;
  const std::string m_stringId;

  Log m_log;
  TradingLog m_tradingLog;

  explicit Implementation(Context &context,
                          const std::string &typeName,
                          const std::string &implementationName,
                          const std::string &instanceName,
                          const ptr::ptree &conf)
      : m_id(uuids::string_generator()(conf.get<std::string>("id"))),
        m_instanceId(nextFreeInstanceId++),
        m_implementationName(implementationName),
        m_instanceName(instanceName),
        m_context(context),
        m_stringId(FormatStringId(
            typeName, m_implementationName, m_instanceName, m_instanceId)),
        m_log(m_stringId, m_context.GetLog()),
        m_tradingLog(m_instanceName, m_context.GetTradingLog()) {
    Assert(nextFreeInstanceId.is_lock_free());
  }
};

Module::Module(Context &context,
               const std::string &typeName,
               const std::string &implementationName,
               const std::string &instanceName,
               const ptr::ptree &conf)
    : m_pimpl(boost::make_unique<Implementation>(
          context, typeName, implementationName, instanceName, conf)) {}

Module::~Module() = default;

const Module::InstanceId &Module::GetInstanceId() const {
  return m_pimpl->m_instanceId;
}

const uuids::uuid &Module::GetId() const { return m_pimpl->m_id; }

const std::string &Module::GetImplementationName() const {
  return m_pimpl->m_implementationName;
}

const std::string &Module::GetInstanceName() const noexcept {
  return m_pimpl->m_instanceName;
}

Module::Lock Module::LockForOtherThreads() { return Lock(m_pimpl->m_mutex); }

const std::string &Module::GetStringId() const noexcept {
  return m_pimpl->m_stringId;
}

Context &Module::GetContext() { return m_pimpl->m_context; }

const Context &Module::GetContext() const {
  return const_cast<Module *>(this)->GetContext();
}

Module::Log &Module::GetLog() const noexcept { return m_pimpl->m_log; }

Module::TradingLog &Module::GetTradingLog() const noexcept {
  return m_pimpl->m_tradingLog;
}

std::string Module::GetRequiredSuppliers() const { return std::string(); }

void Module::OnSettingsUpdate(const ptr::ptree &) {}

void Module::RaiseSettingsUpdateEvent(const ptr::ptree &conf) {
  const auto lock = LockForOtherThreads();
  OnSettingsUpdate(conf);
}

std::ofstream Module::OpenDataLog(const std::string &fileExtension,
                                  const std::string &suffix) const {
  auto path = GetContext().GetSettings().GetLogsDir();
  path /= GetImplementationName();
  {
    boost::format fileName("%1%__%2%__%3%_%4%%5%");
    fileName % GetInstanceName()                          // 1
        % ConvertToFileName(GetContext().GetStartTime())  // 2
        % GetId()                                         // 3
        % GetInstanceId()                                 // 4
        % suffix;                                         // 5
    path /= SymbolToFileName(fileName.str(), fileExtension);
  }

  fs::create_directories(path.branch_path());

  std::ofstream result(path.string(),
                       std::ios::out | std::ios::ate | std::ios::app);
  if (!result) {
    GetLog().Error("Failed to open data log file %1%", path);
    throw Exception("Failed to open data log file");
  } else {
    GetLog().Debug("Data log: %1%.", path);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////

std::ostream &trdk::operator<<(std::ostream &oss, const Module &module) {
  oss << module.GetStringId();
  return oss;
}

//////////////////////////////////////////////////////////////////////////
