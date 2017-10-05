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
#include "ShellEngine.hpp"
#include "Core/RiskControl.hpp"
#include "Engine/Engine.hpp"
#include "ShellDropCopy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd::Shell;

namespace sh = trdk::FrontEnd::Shell;
namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace sig = boost::signals2;

class sh::Engine::Implementation : private boost::noncopyable {
 public:
  sh::Engine &m_self;
  const fs::path m_configFilePath;
  sh::DropCopy m_dropCopy;
  std::unique_ptr<trdk::Engine::Engine> m_engine;
  sig::scoped_connection m_engineLogSubscription;
  TradingSystem::OrderStatusUpdateSlot m_orderTradingSystemSlot;
  boost::array<std::unique_ptr<RiskControlScope>, numberOfTradingModes>
      m_riskControls;

 public:
  explicit Implementation(sh::Engine &self, const fs::path &path)
      : m_self(self),
        m_configFilePath(path),
        m_dropCopy(m_self.parent()),
        m_orderTradingSystemSlot(boost::bind(
            &Implementation::OnOrderUpdate, this, _1, _2, _3, _4, _5, _6)) {
    // Just a smoke-check that config is an engine config:
    IniFile(m_configFilePath).ReadBoolKey("General", "is_replay_mode");

    for (int i = 0; i < numberOfTradingModes; ++i) {
      m_riskControls[i] = boost::make_unique<EmptyRiskControlScope>(
          static_cast<TradingMode>(i), "Front-end");
    }
  }

  void OnContextStateChanged(const Context::State &newState,
                             const std::string *updateMessage) {
    static_assert(Context::numberOfStates == 4, "List changed.");
    switch (newState) {
      case Context::STATE_ENGINE_STARTED:
        emit m_self.StateChanged(true);
        if (updateMessage) {
          emit m_self.Message(tr("Engine started: %1")
                                  .arg(QString::fromStdString(*updateMessage)),
                              false);
        }
        break;

      case Context::STATE_DISPATCHER_TASK_STOPPED_GRACEFULLY:
        emit m_self.StateChanged(false);
        break;
      case Context::STATE_DISPATCHER_TASK_STOPPED_ERROR:
        emit m_self.StateChanged(false);
        // Customized logic for Mrigesh Kejriwal: if error is occurred it
        // restarts engine.
        emit m_self.RestartWanted();
        break;

      case Context::STATE_STRATEGY_BLOCKED:
        if (!updateMessage) {
          emit m_self.Message(tr("Strategy is blocked by unknown reason."),
                              true);
        } else {
          emit m_self.Message(tr("Strategy is blocked: %1.")
                                  .arg(QString::fromStdString(*updateMessage)),
                              true);
        }
        break;
    }
  }

  void OnEngineNewLogRecord(const char *tag,
                            const pt::ptime &time,
                            const std::string *module,
                            const char *message) {
    if (!std::strcmp(tag, "Debug")) {
      return;
    }
    std::ostringstream oss;
    oss << '[' << tag << "]\t" << time;
    if (module) {
      oss << '[' << *module << "] ";
    } else {
      oss << ' ';
    }
    oss << message;
    emit m_self.LogRecord(QString::fromStdString(oss.str()));
  }

  void OnOrderUpdate(const OrderId &id,
                     const std::string &tradingSystemOrderId,
                     const OrderStatus &status,
                     const Qty &remainingQty,
                     const boost::optional<Volume> &,
                     const TradingSystem::TradeInfo *tradeInfo) {
    if (!tradeInfo) {
      emit m_self.Order(static_cast<unsigned int>(id),
                        QString::fromStdString(tradingSystemOrderId),
                        int(status), remainingQty);
    } else {
      emit m_self.Trade(static_cast<unsigned int>(id),
                        QString::fromStdString(tradingSystemOrderId),
                        int(status), remainingQty,
                        QString::fromStdString(tradeInfo->id), tradeInfo->price,
                        tradeInfo->qty);
    }
  }
};

sh::Engine::Engine(const fs::path &path, QWidget *parent)
    : QObject(parent),
      m_pimpl(boost::make_unique<Implementation>(*this, path)) {
  connect(this, &Engine::StateChanged, [this](bool isStarted) {
    if (!isStarted) {
      m_pimpl->m_engine.reset();
    }
  });
  // Customized logic for Mrigesh Kejriwal: if error is occurred it
  // restarts engine.
  connect(this, &Engine::RestartWanted, [this]() {Start();});
}

sh::Engine::~Engine() = default;

const fs::path &sh::Engine::GetConfigFilePath() const {
  return m_pimpl->m_configFilePath;
}

bool sh::Engine::IsStarted() const { return m_pimpl->m_engine ? true : false; }

void sh::Engine::Start() {
  if (m_pimpl->m_engine) {
    throw Exception(tr("Engine already started").toLocal8Bit().constData());
  }
  m_pimpl->m_engine = boost::make_unique<trdk::Engine::Engine>(
      GetConfigFilePath(),
      boost::bind(&Implementation::OnContextStateChanged, &*m_pimpl, _1, _2),
      m_pimpl->m_dropCopy,
      [this](trdk::Engine::Context::Log &log) {
        m_pimpl->m_engineLogSubscription = log.Subscribe(boost::bind(
            &Implementation::OnEngineNewLogRecord, &*m_pimpl, _1, _2, _3, _4));
      },
      boost::unordered_map<std::string, std::string>());
}

void sh::Engine::Stop() {
  if (!m_pimpl->m_engine) {
    throw Exception(tr("Engine is not started").toLocal8Bit().constData());
  }
  m_pimpl->m_engine.reset();
}

Context &sh::Engine::GetContext() {
  if (!m_pimpl->m_engine) {
    throw Exception(tr("Engine is not started").toLocal8Bit().constData());
  }
  return m_pimpl->m_engine->GetContext();
}

const sh::DropCopy &sh::Engine::GetDropCopy() const {
  return m_pimpl->m_dropCopy;
}

const TradingSystem::OrderStatusUpdateSlot &
sh::Engine::GetOrderTradingSystemSlot() {
  return m_pimpl->m_orderTradingSystemSlot;
}
RiskControlScope &sh::Engine::GetRiskControl(const TradingMode &mode) {
  return *m_pimpl->m_riskControls[mode];
}
