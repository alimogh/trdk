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
#include "DropCopy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd;
using namespace trdk::FrontEnd::Lib;

namespace lib = trdk::FrontEnd::Lib;
namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace sig = boost::signals2;
namespace ids = boost::uuids;

class lib::Engine::Implementation : private boost::noncopyable {
 public:
  lib::Engine &m_self;
  const fs::path m_configFilePath;
  lib::DropCopy m_dropCopy;
  std::unique_ptr<trdk::Engine::Engine> m_engine;
  sig::scoped_connection m_engineLogSubscription;
  boost::array<std::unique_ptr<RiskControlScope>, numberOfTradingModes>
      m_riskControls;

 public:
  explicit Implementation(lib::Engine &self, const fs::path &path)
      : m_self(self), m_configFilePath(path), m_dropCopy(m_self.parent()) {
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
      case Context::STATE_DISPATCHER_TASK_STOPPED_ERROR:
        emit m_self.StateChanged(false);
        break;

      case Context::STATE_STRATEGY_BLOCKED:
        if (!updateMessage) {
          emit m_self.Message(tr("Strategy is blocked by unknown reason."),
                              true);
        } else {
          emit m_self.Message(tr("Strategy is blocked: \"%1\".")
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

#ifdef DEV_VER

  boost::shared_future<void> m_testFuture;
  boost::shared_ptr<Dummies::Strategy> m_testStrategy;

  void Test() {
    m_testFuture = boost::async([this]() {
      try {
        return RunTest();
      } catch (const std::exception &ex) {
        QMessageBox::critical(nullptr, tr("Debug test error"), ex.what(),
                              QMessageBox::Ok);
      }
    });
  }
  void RunTest() {
    ids::random_generator generateUuid;
    const auto operationId = generateUuid();
    m_testStrategy = boost::make_shared<Dummies::Strategy>(m_self.GetContext());
    Security *security1 = nullptr;
    Security *security2 = nullptr;
    m_self.GetContext().GetMarketDataSource(0).ForEachSecurity(
        [&security1, &security2](Security &source) {
          if (!security1) {
            security1 = &source;
          } else if (!security2) {
            security2 = &source;
          }
        });
    Assert(security1);
    Assert(security2);

    for (size_t i = 0; i < 3; ++i) {
      boost::this_thread::sleep(pt::seconds(1));
      const auto &operation = boost::make_shared<Operation>(
          *m_testStrategy,
          boost::make_unique<TradingLib::PnlOneSymbolContainer>());
      if (i && !(i % 3)) {
        continue;
      }
      operation->UpdatePnl(*security1, ORDER_SIDE_BUY, 10, 1, 0.00001);
      operation->UpdatePnl(*security1, ORDER_SIDE_SELL, 10, i % 2 ? 0.9 : 1.1,
                           0.00001);
      operation->UpdatePnl(*security2, ORDER_SIDE_BUY, 1.23124 * i, 1.23124 * i,
                           0.00001);
      operation->UpdatePnl(*security2, ORDER_SIDE_SELL, 1.23124 * i,
                           1.23124 * i, 0.00001);
      for (size_t j = 0; j < 6; ++j) {
        const auto &tradingSystem =
            m_self.GetContext().GetTradingSystem(0, TRADING_MODE_LIVE);
        const auto position = boost::make_shared<LongPosition>(
            operation, j + 1, *security1, security1->GetSymbol().GetCurrency(),
            2312313, 62423, TimeMeasurement::Milestones());
        const auto &orderId = boost::lexical_cast<std::string>(generateUuid());
        const auto qty = 100.00 * (j + 1) * (i + 1);
        m_dropCopy.CopySubmittedOrder(
            orderId, m_self.GetContext().GetCurrentTime(), *position,
            j % 2 ? ORDER_SIDE_SELL : ORDER_SIDE_BUY, qty, Price(12.345678901),
            j % 2 ? TIME_IN_FORCE_GTC : TIME_IN_FORCE_IOC);
        boost::this_thread::sleep(pt::seconds(1));
        if (j < 5) {
          m_dropCopy.CopyOrderStatus(
              orderId, tradingSystem, m_self.GetContext().GetCurrentTime(),
              j <= 1 || (j == 2 && (i != 2 || i % 5))
                  ? ORDER_STATUS_FILLED_FULLY
                  : j == 2 ? ORDER_STATUS_ERROR
                           : j == 3 ? ORDER_STATUS_CANCELED
                                    : ORDER_STATUS_REJECTED,
              j == 0 ? 0 : 55.55 * (i + 1));
        } else {
          m_dropCopy.CopyOrderStatus(
              orderId, tradingSystem, m_self.GetContext().GetCurrentTime(),
              j == 4 ? ORDER_STATUS_CANCELED : ORDER_STATUS_REJECTED, qty);
        }
      }
    }
  }
#endif
};

lib::Engine::Engine(const fs::path &path, QWidget *parent)
    : QObject(parent),
      m_pimpl(boost::make_unique<Implementation>(*this, path)) {
  Verify(connect(this, &Engine::StateChanged, [this](bool isStarted) {
    if (!isStarted) {
      m_pimpl->m_engine.reset();
    }
  }));
}

lib::Engine::~Engine() {
  // Fixes second stop by StateChanged-signal.
  m_pimpl->m_engine.reset();
}

const fs::path &lib::Engine::GetConfigFilePath() const {
  return m_pimpl->m_configFilePath;
}

bool lib::Engine::IsStarted() const { return m_pimpl->m_engine ? true : false; }

void lib::Engine::Start(
    const boost::function<void(const std::string &)> &startProgressCallback) {
  if (m_pimpl->m_engine) {
    throw Exception(tr("Engine already started").toLocal8Bit().constData());
  }
  m_pimpl->m_engine = boost::make_unique<trdk::Engine::Engine>(
      GetConfigFilePath(),
      boost::bind(&Implementation::OnContextStateChanged, &*m_pimpl, _1, _2),
      m_pimpl->m_dropCopy, startProgressCallback,
      [this](const std::string &error) {
        return QMessageBox::critical(
                   nullptr, tr("Error at engine starting"),
                   QString::fromStdString(error.substr(0, 512)),
                   QMessageBox::Retry | QMessageBox::Ignore) ==
               QMessageBox::Ignore;
      },
      [this](trdk::Engine::Context::Log &log) {
        m_pimpl->m_engineLogSubscription = log.Subscribe(boost::bind(
            &Implementation::OnEngineNewLogRecord, &*m_pimpl, _1, _2, _3, _4));
      },
      boost::unordered_map<std::string, std::string>());
}

void lib::Engine::Stop() {
  if (!m_pimpl->m_engine) {
    throw Exception(tr("Engine is not started").toLocal8Bit().constData());
  }
  m_pimpl->m_engine.reset();
}

Context &lib::Engine::GetContext() {
  if (!m_pimpl->m_engine) {
    throw Exception(tr("Engine is not started").toLocal8Bit().constData());
  }
  return m_pimpl->m_engine->GetContext();
}

const lib::DropCopy &lib::Engine::GetDropCopy() const {
  return m_pimpl->m_dropCopy;
}

RiskControlScope &lib::Engine::GetRiskControl(const TradingMode &mode) {
  return *m_pimpl->m_riskControls[mode];
}

#ifdef DEV_VER
void lib::Engine::Test() { m_pimpl->Test(); }
#endif
