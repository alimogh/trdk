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
#include "OperationOrm.hpp"
#include "OperationRecord.hpp"
#include "OperationStatus.hpp"
#include "OrderOrm.hpp"
#include "PnlOrm.hpp"
#include "StrategyInstanceOrm.hpp"

#ifdef DEV_VER
#define TRDK_FRONTENT_HAS_DB_TRACING
#endif

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;
namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;
namespace json = ptr::json_parser;
namespace sig = boost::signals2;

namespace {

OperationStatus GetOperationStatus(const Pnl::Result& source) {
  static_assert(Pnl::numberOfResults == 5, "List changed");
  switch (source) {
    case Pnl::RESULT_NONE:
      return OperationStatus::Active;
    case Pnl::RESULT_COMPLETED:
      return OperationStatus::Completed;
    case Pnl::RESULT_PROFIT:
      return OperationStatus::Profit;
    case Pnl::RESULT_LOSS:
      return OperationStatus::Loss;
    case Pnl::RESULT_ERROR:
      return OperationStatus::Error;
    default:
      AssertEq(Pnl::RESULT_NONE, source);
      throw Exception("Unknown operation result");
  }
}

#ifdef TRDK_FRONTENT_HAS_DB_TRACING
class DbTracer : public odb::tracer {
 public:
  explicit DbTracer(const std::unique_ptr<trdk::Engine::Engine>& engine)
      : m_engine(engine) {}
  DbTracer(DbTracer&&) = default;
  DbTracer(const DbTracer&) = delete;
  DbTracer& operator=(DbTracer&&) = delete;
  DbTracer& operator=(const DbTracer&) = delete;
  ~DbTracer() override = default;

  void execute(odb::connection&, const char* statement) override {
    if (!m_engine) {
      return;
    }
    m_engine->GetContext().GetLog().Debug("DB: %1%", statement);
  }

 private:
  const std::unique_ptr<trdk::Engine::Engine>& m_engine;
};
#endif

}  // namespace

class FrontEnd::Engine::Implementation : boost::noncopyable {
 public:
  FrontEnd::Engine& m_self;
  const fs::path m_configFile;
  const fs::path m_logsDir;
  FrontEnd::DropCopy m_dropCopy;
  std::unique_ptr<trdk::Engine::Engine> m_engine;
  sig::scoped_connection m_engineLogSubscription;
  boost::array<std::unique_ptr<RiskControlScope>, numberOfTradingModes>
      m_riskControls;

#ifdef TRDK_FRONTENT_HAS_DB_TRACING
  DbTracer m_dbTracer;
#endif
  std::unique_ptr<odb::database> m_db;

  explicit Implementation(FrontEnd::Engine& self,
                          fs::path configFile,
                          fs::path logsDir) try
      : m_self(self),
        m_configFile(std::move(configFile)),
        m_logsDir(std::move(logsDir)),
        m_dropCopy(m_self.parent()),
#ifdef TRDK_FRONTENT_HAS_DB_TRACING
        m_dbTracer(m_engine),
#endif
        m_db(CreateDb()) {
    for (auto i = 0; i < numberOfTradingModes; ++i) {
      m_riskControls[i] = boost::make_unique<EmptyRiskControlScope>(
          static_cast<TradingMode>(i), "Front-end");
    }
  } catch (const odb::exception& ex) {
    const auto& error =
        tr(R"(Database initialization error: "%1")").arg(ex.what());
    throw Exception(error.toStdString().c_str());
  }

  std::unique_ptr<odb::database> CreateDb() {
    const auto& storagePath =
        GetStandardFilePath("trading.db", QStandardPaths::DataLocation);
    const auto isStorageExistent = fs::exists(storagePath);

    auto result = boost::make_unique<odb::sqlite::database>(
        storagePath.string(), SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
#ifdef TRDK_FRONTENT_HAS_DB_TRACING
    result->tracer(m_dbTracer);
#endif

    if (result->schema_version()) {
      return result;
    }

    if (isStorageExistent) {
      const auto& error = tr(R"(Database storage file "%1" has unknown format)")
                              .arg(storagePath.string().c_str());
      throw Exception(error.toStdString().c_str());
    }

    // Create the database schema. Due to bugs in SQLite foreign key support for
    // DDL statements, we need to temporarily disable foreign keys. Also  see
    // odb::create_database.
    const auto connection = result->connection();
    connection->execute("PRAGMA foreign_keys=OFF");
    odb::transaction transaction(connection->begin());
    odb::schema_catalog::create_schema(*result);
    transaction.commit();
    connection->execute("PRAGMA foreign_keys=ON");
    return result;
  }

  void ConnectSignals() {
    Verify(connect(&m_self, &Engine::StateChange, [this](const bool isStarted) {
      if (!isStarted) {
        m_engine.reset();
      }
    }));

    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::OperationStarted, &m_self,
        boost::bind(&Implementation::StartOperation, this, _1, _2, _3),
        Qt::QueuedConnection));
    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::OperationUpdated, &m_self,
        boost::bind(&Implementation::UpdateOperation, this, _1, _2),
        Qt::QueuedConnection));
    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::OperationCompleted, &m_self,
        boost::bind(&Implementation::CompleteOperation, this, _1, _2, _3),
        Qt::QueuedConnection));
    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::OperationOrderSubmited, &m_self,
        [this](const QUuid& operationId, const int64_t subOperationId,
               QString orderRemoteId, QDateTime submitTime,
               const Security* security,
               const boost::shared_ptr<const Currency>& currency,
               const TradingSystem* tradingSystem,
               const boost::shared_ptr<const OrderSide>& side, const Qty& qty,
               boost::optional<Price> price, const TimeInForce& timeInForce) {
          SubmitOperationOrder(operationId, subOperationId,
                               std::move(orderRemoteId), std::move(submitTime),
                               *security, *currency, *tradingSystem, *side, qty,
                               std::move(price), timeInForce, OrderStatus::Sent,
                               boost::none);
        },
        Qt::QueuedConnection));
    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::OperationOrderSubmitError, &m_self,
        [this](const QUuid& operationId, const int64_t subOperationId,
               QDateTime submitTime, const Security* security,
               const boost::shared_ptr<const Lib::Currency>& currency,
               const TradingSystem* tradingSystem,
               const boost::shared_ptr<const OrderSide>& side, const Qty& qty,
               boost::optional<Price> price, const TimeInForce& timeInForce,
               QString error) {
          SubmitOperationOrder(
              operationId, subOperationId, QUuid::createUuid().toString(),
              std::move(submitTime), *security, *currency, *tradingSystem,
              *side, qty, std::move(price), timeInForce, OrderStatus::Error,
              std::move(error));
        },
        Qt::QueuedConnection));
    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::FreeOrderSubmited, &m_self,
        [this](QString remoteId, QDateTime submitTime, const Security* security,
               const boost::shared_ptr<const Lib::Currency>& currency,
               const TradingSystem* tradingSystem,
               const boost::shared_ptr<const OrderSide>& side, const Qty& qty,
               boost::optional<Price> price, const TimeInForce& timeInForce) {
          SubmitFreeOrder(std::move(remoteId), std::move(submitTime), *security,
                          *currency, *tradingSystem, *side, std::move(qty),
                          std::move(price), timeInForce, OrderStatus::Sent,
                          boost::none);
        },
        Qt::QueuedConnection));
    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::FreeOrderSubmitError, &m_self,
        [this](QDateTime submitTime, const Security* security,
               const boost::shared_ptr<const Currency>& currency,
               const TradingSystem* tradingSystem,
               const boost::shared_ptr<const OrderSide>& side, const Qty& qty,
               boost::optional<Price> price, const TimeInForce& timeInForce,
               QString error) {
          SubmitFreeOrder(QUuid::createUuid().toString(), std::move(submitTime),
                          *security, *currency, *tradingSystem, *side, qty,
                          std::move(price), timeInForce, OrderStatus::Error,
                          std::move(error));
        },
        Qt::QueuedConnection));
    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::OrderUpdated, &m_self,
        boost::bind(&Implementation::UpdateOrder, this, _1, _2, _3, _4, _5),
        Qt::QueuedConnection));

    Verify(m_self.connect(&m_dropCopy, &DropCopy::PriceUpdated, &m_self,
                          [this](const Security* security) {
                            emit m_self.PriceUpdated(security);
                          },
                          Qt::QueuedConnection));

    qRegisterMetaType<Bar>("sFrontEnd::Bar");
    Verify(m_self.connect(&m_dropCopy, &DropCopy::BarUpdated, &m_self,
                          [this](const Security* security, const Bar& bar) {
                            emit m_self.BarUpdated(security, bar);
                          },
                          Qt::QueuedConnection));
  }

  void ChangeContextState(const Context::State& newState,
                          const std::string* updateMessage) {
    static_assert(Context::numberOfStates == 4, "List changed.");
    switch (newState) {
      case Context::STATE_ENGINE_STARTED:
        emit m_self.StateChange(true);
        if (updateMessage) {
          emit m_self.Message(tr("Engine started: %1")
                                  .arg(QString::fromStdString(*updateMessage)),
                              false);
        }
        break;

      case Context::STATE_DISPATCHER_TASK_STOPPED_GRACEFULLY:
      case Context::STATE_DISPATCHER_TASK_STOPPED_ERROR:
        emit m_self.StateChange(false);
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
      default:
        break;
    }
  }

  void LogEngineEvent(const char* tag,
                      const pt::ptime& time,
                      const std::string* module,
                      const char* message) {
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

  void SubmitOperationOrder(const QUuid& operationId,
                            int64_t subOperationId,
                            QString orderRemoteId,
                            QDateTime submitTime,
                            const Security& security,
                            const Currency& currency,
                            const TradingSystem& tradingSystem,
                            const OrderSide& side,
                            const Qty& qty,
                            boost::optional<Price> price,
                            const TimeInForce& timeInForce,
                            const OrderStatus& status,
                            boost::optional<QString> additionalInfo) {
    boost::shared_ptr<OrderRecord> order;
    {
      odb::transaction transaction(m_db->begin());
      auto operation = m_db->load<OperationRecord>(operationId);
      order = boost::make_shared<OrderRecord>(
          std::move(orderRemoteId), subOperationId, operation,
          QString::fromStdString(security.GetSymbol().GetSymbol()), currency,
          QString::fromStdString(tradingSystem.GetInstanceName()), side, qty,
          std::move(price), timeInForce, std::move(submitTime), status);
      order->SetAdditionalInfo(std::move(additionalInfo));
      m_db->persist(order);
      operation->AddOrder(order);
      m_db->update(operation);
      transaction.commit();
    }
    emit m_self.OrderUpdated(order);
  }

  void SubmitFreeOrder(QString remoteId,
                       QDateTime submitTime,
                       const Security& security,
                       const Currency& currency,
                       const TradingSystem& tradingSystem,
                       const OrderSide& orderSide,
                       const Qty& qty,
                       boost::optional<Price> price,
                       const TimeInForce& timeInForce,
                       const OrderStatus& status,
                       boost::optional<QString> additionalInfo) {
    auto order = boost::make_shared<OrderRecord>(
        std::move(remoteId),
        QString::fromStdString(security.GetSymbol().GetSymbol()), currency,
        QString::fromStdString(tradingSystem.GetInstanceName()), orderSide, qty,
        std::move(price), timeInForce, std::move(submitTime), status);
    order->SetAdditionalInfo(std::move(additionalInfo));
    {
      odb::transaction transaction(m_db->begin());
      m_db->persist(*order);
      transaction.commit();
    }
    emit m_self.OrderUpdated(order);
  }

  void UpdateOrder(const QString& id,
                   const TradingSystem* tradingSystem,
                   QDateTime time,
                   const boost::shared_ptr<const OrderStatus>& status,
                   const Qty& remainingQty) {
    odb::transaction transaction(m_db->begin());
    const auto& order = m_db->query_one<OrderRecord>(
        OrderQuery::remoteId == id &&
        OrderQuery::tradingSystemInstance ==
            QString::fromStdString(tradingSystem->GetInstanceName()));
    if (!order) {
      m_self.LogError(
          QString("Failed to find order record in DB with remote ID \"%1\" and "
                  "trading system instance \"%2\" to update order.")
              .arg(id,  // 1
                   QString::fromStdString(
                       tradingSystem->GetInstanceName())));  // 2
      Assert(order);
      return;
    }
    order->SetUpdateTime(std::move(time));
    order->SetStatus(*status);
    order->SetRemainingQty(remainingQty);
    m_db->update(order);
    transaction.commit();
    emit m_self.OrderUpdated(order);
  }

  void StartOperation(const QUuid& id,
                      const QDateTime& time,
                      const Strategy* strategySource) {
    boost::shared_ptr<OperationRecord> operation;
    {
      odb::transaction transaction(m_db->begin());
      const auto& strategy = m_db->load<StrategyInstanceRecord>(
          ConvertToQUuid(strategySource->GetId()));
      operation = boost::make_shared<OperationRecord>(id, time, strategy,
                                                      OperationStatus::Active);
      strategy->AddOperation(operation);
      m_db->persist(operation);
      m_db->update(strategy);
      transaction.commit();
    }
    emit m_self.OperationUpdated(operation);
  }

  void UpdateOperation(const QUuid& id, const Pnl::Data& pnl) {
    odb::transaction transaction(m_db->begin());
    const auto& operation = m_db->load<OperationRecord>(id);
    UpdatePnl(operation, pnl);
    m_db->update(operation);
    transaction.commit();
    emit m_self.OperationUpdated(operation);
  }

  void CompleteOperation(const QUuid& id,
                         const QDateTime& time,
                         const boost::shared_ptr<const Pnl>& pnl) {
    odb::transaction transaction(m_db->begin());
    auto operation = m_db->load<OperationRecord>(id);
    operation->SetEndTime(time);
    operation->SetStatus(GetOperationStatus(pnl->GetResult()));
    UpdatePnl(operation, pnl->GetData());
    m_db->update(operation);
    transaction.commit();
    emit m_self.OperationUpdated(operation);
  }

  void UpdatePnl(const boost::shared_ptr<OperationRecord>& operation,
                 const Pnl::Data& pnlSet) {
    for (const auto& pnlUpdate : pnlSet) {
      auto isExistinet = false;
      for (auto& operationPnlPtr : operation->GetPnl()) {
        const auto& operationPnl = operationPnlPtr.lock();
        if (operationPnl->GetSymbol() == pnlUpdate.first.c_str()) {
          auto record = boost::make_shared<PnlRecord>(*operationPnl);
          record->SetFinancialResult(pnlUpdate.second.financialResult);
          record->SetCommission(pnlUpdate.second.commission);
          operationPnlPtr = record;
          m_db->update(record);
          isExistinet = true;
          break;
        }
      }
      if (isExistinet) {
        continue;
      }
      auto record = boost::make_shared<PnlRecord>(
          QString::fromStdString(pnlUpdate.first), operation);
      record->SetFinancialResult(pnlUpdate.second.financialResult);
      record->SetCommission(pnlUpdate.second.commission);
      m_db->persist(record);
      operation->GetPnl().emplace_back(std::move(record));
    }
  }
};

FrontEnd::Engine::Engine(const fs::path& configFile,
                         const fs::path& logsDir,
                         QWidget* parent)
    : QObject(parent),
      m_pimpl(boost::make_unique<Implementation>(*this, configFile, logsDir)) {
  m_pimpl->ConnectSignals();
}

FrontEnd::Engine::~Engine() {
  // Fixes second stop by StateChanged-signal.
  m_pimpl->m_engine.reset();
}

bool FrontEnd::Engine::IsStarted() const {
  return static_cast<bool>(m_pimpl->m_engine);
}

void FrontEnd::Engine::Start(
    const boost::function<void(const std::string&)>& startProgressCallback) {
  if (m_pimpl->m_engine) {
    throw Exception(tr("Engine already started").toLocal8Bit().constData());
  }
  m_pimpl->m_engine = boost::make_unique<trdk::Engine::Engine>(
      m_pimpl->m_configFile, m_pimpl->m_logsDir,
      boost::bind(&Implementation::ChangeContextState, &*m_pimpl, _1, _2),
      m_pimpl->m_dropCopy, startProgressCallback,
      [](const std ::string& error) {
        return QMessageBox::critical(
                   nullptr, tr("Error at engine starting"),
                   QString::fromStdString(error.substr(0, 512)),
                   QMessageBox::Retry | QMessageBox::Ignore) ==
               QMessageBox::Ignore;
      },
      [this](Context::Log& log) {
        m_pimpl->m_engineLogSubscription = log.Subscribe(boost::bind(
            &Implementation::LogEngineEvent, &*m_pimpl, _1, _2, _3, _4));
      });
}

void FrontEnd::Engine::Stop() {
  if (!m_pimpl->m_engine) {
    throw Exception(tr("Engine is not started").toLocal8Bit().constData());
  }
  m_pimpl->m_engine.reset();
}

Context& FrontEnd::Engine::GetContext() {
  if (!m_pimpl->m_engine) {
    throw Exception(tr("Engine is not started").toLocal8Bit().constData());
  }
  return m_pimpl->m_engine->GetContext();
}

const Context& FrontEnd::Engine::GetContext() const {
  return const_cast<Engine*>(this)->GetContext();
}

const FrontEnd::DropCopy& FrontEnd::Engine::GetDropCopy() const {
  return m_pimpl->m_dropCopy;
}

RiskControlScope& FrontEnd::Engine::GetRiskControl(const TradingMode& mode) {
  return *m_pimpl->m_riskControls[mode];
}

std::vector<boost::shared_ptr<OperationRecord>> FrontEnd::Engine::GetOperations(
    const QDateTime& startTime,
    const boost::optional<QDateTime>& endTime,
    const bool isTradesIncluded,
    const bool isErrorsIncluded,
    const bool isCancelsIncluded,
    const boost::optional<QString>& strategy) const {
  auto startTimeCondition = OperationQuery::startTime >= startTime;
  auto endTimeCondition = OperationQuery::endTime >= startTime;
  if (endTime) {
    startTimeCondition =
        startTimeCondition && OperationQuery::startTime <= *endTime;
    endTimeCondition = endTimeCondition && OperationQuery::endTime <= *endTime;
  }

  auto query = startTimeCondition || endTimeCondition;

  if (strategy) {
    query = query && StrategyInstanceQuery::name == *strategy;
  }

  if (!isTradesIncluded || !isErrorsIncluded || !isCancelsIncluded) {
    std::vector<std::string> list;
    if (!isTradesIncluded) {
      list.emplace_back(boost::lexical_cast<std::string>(
          (+OperationStatus::Loss)._to_integral()));
      list.emplace_back(boost::lexical_cast<std::string>(
          (+OperationStatus::Profit)._to_integral()));
    }
    if (!isErrorsIncluded) {
      list.emplace_back(boost::lexical_cast<std::string>(
          (+OperationStatus::Error)._to_integral()));
    }
    if (!isCancelsIncluded) {
      list.emplace_back(boost::lexical_cast<std::string>(
          (+OperationStatus::Canceled)._to_integral()));
    }
    query = query &&
            OperationQuery("status NOT IN (" + boost::join(list, ", ") + ')');
  }

  std::vector<boost::shared_ptr<OperationRecord>> result;
  {
    odb::transaction transaction(m_pimpl->m_db->begin());
    for (auto& record : m_pimpl->m_db->query<OperationRecord>(query)) {
      result.emplace_back(
          boost::make_shared<OperationRecord>(std::move(record)));
    }
    transaction.commit();
  }

  return result;
}

ptr::ptree FrontEnd::Engine::LoadConfig() const {
  std::ifstream file(m_pimpl->m_configFile.c_str());
  if (!file) {
    throw Exception(
        tr("Filed to read configuration file").toStdString().c_str());
  }
  ptr::ptree result;
  try {
    json::read_json(file, result);
  } catch (const ptr::json_parser_error&) {
    throw Exception(
        tr("Configuration file has invalid format").toStdString().c_str());
  }
  return result;
}

void FrontEnd::Engine::StoreConfig(const ptr::ptree& config) {
  std::ofstream file(m_pimpl->m_configFile.c_str());
  if (!file) {
    throw Exception(
        tr("Filed to write configuration file").toStdString().c_str());
  }
  json::write_json(file, config, true);
}

void FrontEnd::Engine::StoreConfig(const Strategy& strategy,
                                   ptr::ptree config,
                                   const bool isActive) {
  //! @todo Don't hide strategy instance record, use StrategyInstanceRecord
  //! for clients to create and update info.
  try {
    odb::transaction transaction(m_pimpl->m_db->begin());
    auto record = m_pimpl->m_db->find<StrategyInstanceRecord>(
        ConvertToQUuid(strategy.GetId()));
    const auto isNew = !record;
    if (isNew) {
      record = boost::make_shared<StrategyInstanceRecord>(
          ConvertToQUuid(strategy.GetId()),
          ConvertToQUuid(strategy.GetTypeId()));
    }
    record->SetName(QString::fromStdString(strategy.GetInstanceName()));
    record->SetActive(isActive);
    record->SetConfig(std::move(config));
    isNew ? m_pimpl->m_db->persist(record) : m_pimpl->m_db->update(record);
    transaction.commit();
  } catch (const std::exception& ex) {
    m_pimpl->m_engine->GetContext().GetLog().Error(
        "Failed to store strategy instance config for strategy %1%: "
        "\"%2%\".",
        strategy.GetId(),  // 1
        ex.what());        // 2
  }
}

void FrontEnd::Engine::ForEachActiveStrategy(
    const boost::function<void(const QUuid& typeId,
                               const QUuid& instanceId,
                               const QString& name,
                               const ptr::ptree& config)>& callback) const {
  odb::transaction transaction(m_pimpl->m_db->begin());
  try {
    for (const auto& instance : m_pimpl->m_db->query<StrategyInstanceRecord>(
             StrategyInstanceQuery::isActive == true)) {
      callback(instance.GetTypeId(), instance.GetId(), instance.GetName(),
               instance.GetConfig());
    }
  } catch (const std::exception& ex) {
    m_pimpl->m_engine->GetContext().GetLog().Error(
        "Failed to load active strategy instance list: \"%1%\".", ex.what());
    return;
  }
  transaction.commit();
}

std::vector<QString> FrontEnd::Engine::GetStrategyNameList() const {
  std::vector<QString> result;
  odb::transaction transaction(m_pimpl->m_db->begin());
  for (const auto& name : m_pimpl->m_db->query<StrategyInstanceName>()) {
    result.emplace_back(name.name);
  }
  transaction.commit();
  return result;
}

std::string FrontEnd::Engine::GenerateNewStrategyInstanceName(
    const std::string& nameBase) {
  return nameBase;
}

void FrontEnd::Engine::LogDebug(const QString& message) {
  GetContext().GetLog().Debug(message.toStdString().c_str());
}
void FrontEnd::Engine::LogInfo(const QString& message) {
  GetContext().GetLog().Info(message.toStdString().c_str());
}
void FrontEnd::Engine::LogWarn(const QString& message) {
  GetContext().GetLog().Warn(message.toStdString().c_str());
}
void FrontEnd::Engine::LogError(const QString& message) {
  GetContext().GetLog().Error(message.toStdString().c_str());
}

QString FrontEnd::Engine::ResolveTradingSystemTitle(
    const QString& instanceNameSource) const {
  const auto& instanceName = instanceNameSource.toStdString();
  const auto& size = GetContext().GetNumberOfTradingSystems();
  for (size_t i = 0; i < size; ++i) {
    const auto& tradingSystem =
        GetContext().GetTradingSystem(i, TRADING_MODE_LIVE);
    if (tradingSystem.GetInstanceName() == instanceName) {
      return QString::fromStdString(tradingSystem.GetTitle());
    }
  }
  return instanceNameSource;
}
