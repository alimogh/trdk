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

namespace front = trdk::FrontEnd;
namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace sig = boost::signals2;
namespace ids = boost::uuids;
namespace db = qx::dao;

namespace {
Orm::OperationStatus::enum_OperationStatus CovertToOperationStatus(
    const Pnl::Result &source) {
  static_assert(Pnl::numberOfResults == 4, "List changed.");
  switch (source) {
    case Pnl::RESULT_NONE:
      return Orm::OperationStatus::CANCELED;
      break;
    case Pnl::RESULT_PROFIT:
      return Orm::OperationStatus::PROFIT;
      break;
    case Pnl::RESULT_LOSS:
      return Orm::OperationStatus::LOSS;
      break;
    default:
      AssertEq(Pnl::RESULT_ERROR, source);
    case Pnl::RESULT_ERROR:
      return Orm::OperationStatus::ERROR;
      break;
  }
}
}  // namespace

class front::Engine::Implementation : private boost::noncopyable {
 public:
  front::Engine &m_self;
  const fs::path m_configFilePath;
  front::DropCopy m_dropCopy;
  std::unique_ptr<trdk::Engine::Engine> m_engine;
  sig::scoped_connection m_engineLogSubscription;
  boost::array<std::unique_ptr<RiskControlScope>, numberOfTradingModes>
      m_riskControls;
  QSqlDatabase *const m_db;

 public:
  explicit Implementation(front::Engine &self, const fs::path &path)
      : m_self(self),
        m_configFilePath(path),
        m_dropCopy(m_self.parent()),
        m_db(nullptr) {
    // Just a smoke-check that config is an engine config:
    IniFile(m_configFilePath).ReadBoolKey("General", "is_replay_mode");

    for (int i = 0; i < numberOfTradingModes; ++i) {
      m_riskControls[i] = boost::make_unique<EmptyRiskControlScope>(
          static_cast<TradingMode>(i), "Front-end");
    }
  }

  void InitDb() {
    qx::QxSqlDatabase::getSingleton()->setDriverName("QSQLITE");
    qx::QxSqlDatabase::getSingleton()->setDatabaseName(QString::fromStdString(
        (m_configFilePath.branch_path() / "default.db").string()));
    qx::QxSqlDatabase::getSingleton()->setHostName("localhost");
    qx::QxSqlDatabase::getSingleton()->setUserName("root");
    qx::QxSqlDatabase::getSingleton()->setPassword("");

    db::create_table<Orm::Strategy>(m_db);
    db::create_table<Orm::Operation>(m_db);
    db::create_table<Orm::Order>(m_db);
    db::create_table<Orm::Pnl>(m_db);
  }

  void ConnectSignals() {
    Verify(connect(&m_self, &Engine::StateChange, [this](bool isStarted) {
      if (!isStarted) {
        m_engine.reset();
      }
    }));

    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::OperationStart, &m_self,
        boost::bind(&Implementation::OnOperationStart, this, _1, _2, _3),
        Qt::QueuedConnection));
    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::OperationUpdate, &m_self,
        boost::bind(&Implementation::OnOperationUpdate, this, _1, _2),
        Qt::QueuedConnection));
    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::OperationEnd, &m_self,
        boost::bind(&Implementation::OnOperationEnd, this, _1, _2, _3),
        Qt::QueuedConnection));
    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::OperationOrderSubmit, &m_self,
        [this](const ids::uuid &operationId, int64_t subOperationId,
               const OrderId &id, const pt::ptime &orderTime,
               const Security *security, const Currency &currency,
               const TradingSystem *tradingSystem, const OrderSide &orderSide,
               const Qty &qty, const boost::optional<Price> &price,
               const TimeInForce &timeInForce) {
          OnOperationOrderSubmit(std::make_pair(operationId, subOperationId),
                                 id, orderTime, *security, currency,
                                 *tradingSystem, orderSide, qty, price,
                                 timeInForce);
        },
        Qt::QueuedConnection));
    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::OperationOrderSubmitError, &m_self,
        [this](const ids::uuid &operationId, int64_t subOperationId,
               const pt::ptime &orderTime, const Security *security,
               const Currency &currency, const TradingSystem *tradingSystem,
               const OrderSide &side, const Qty &qty,
               const boost::optional<Price> &price,
               const TimeInForce &timeInForce, const QString &error) {
          OnOperationOrderSubmitError(
              std::make_pair(operationId, subOperationId), orderTime, *security,
              currency, *tradingSystem, side, qty, price, timeInForce, error);
        },
        Qt::QueuedConnection));
    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::FreeOrderSubmit, &m_self,
        [this](const OrderId &id, const pt::ptime &orderTime,
               const Security *security, const Currency &currency,
               const TradingSystem *tradingSystem, const OrderSide &orderSide,
               const Qty &qty, const boost::optional<Price> &price,
               const TimeInForce &timeInForce) {
          OnOperationOrderSubmit(boost::none, id, orderTime, *security,
                                 currency, *tradingSystem, orderSide, qty,
                                 price, timeInForce);
        },
        Qt::QueuedConnection));
    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::FreeOrderSubmitError, &m_self,
        [this](const pt::ptime &orderTime, const Security *security,
               const Currency &currency, const TradingSystem *tradingSystem,
               const OrderSide &side, const Qty &qty,
               const boost::optional<Price> &price,
               const TimeInForce &timeInForce, const QString &error) {
          OnOperationOrderSubmitError(boost::none, orderTime, *security,
                                      currency, *tradingSystem, side, qty,
                                      price, timeInForce, error);
        },
        Qt::QueuedConnection));
    Verify(m_self.connect(
        &m_dropCopy, &DropCopy::OrderUpdate, &m_self,
        boost::bind(&Implementation::OnOrderUpdate, this, _1, _2, _3, _4, _5),
        Qt::QueuedConnection));
  }

  void OnContextStateChanged(const Context::State &newState,
                             const std::string *updateMessage) {
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

  void OnNewOrder(const boost::optional<std::pair<ids::uuid, int64_t>>
                      &operationIdSuboperationId,
                  QString &&remoteId,
                  const pt::ptime &time,
                  const Security &security,
                  const Currency &currency,
                  const TradingSystem &tradingSystem,
                  const OrderSide &side,
                  const Qty &qty,
                  const boost::optional<Price> &price,
                  const OrderStatus &status,
                  const QString &additionalInfo) {
    Orm::Order order;
    if (operationIdSuboperationId) {
      {
        auto operation = boost::make_shared<Orm::Operation>(
            ConvertToQUuid(operationIdSuboperationId->first));
        db::fetch_by_id(operation, m_db);
        order.setOperation(operation);
      }
      order.setSubOperationId(operationIdSuboperationId->second);
    }
    order.setRemoteId(std::move(remoteId));
    order.setOrderTime(ConvertToDbDateTime(time));
    order.setSymbol(QString::fromStdString(security.GetSymbol().GetSymbol()));
    order.setCurrency(QString::fromStdString(ConvertToIso(currency)));
    order.setTradingSystem(
        QString::fromStdString(tradingSystem.GetInstanceName()));
    order.setIsBuy(side == ORDER_SIDE_BUY);
    order.setQty(qty);
    order.setRemainingQty(qty);
    order.setPrice(price.get_value_or(0));
    order.setStatus(status);
    order.setAdditionalInfo(additionalInfo);
    db::insert(order, m_db);
    emit m_self.OrderUpdate(order);
  }

  void OnOperationOrderSubmit(
      const boost::optional<std::pair<ids::uuid, int64_t>>
          &operationIdSuboperationId,
      const OrderId &id,
      const pt::ptime &time,
      const Security &security,
      const Currency &currency,
      const TradingSystem &tradingSystem,
      const OrderSide &side,
      const Qty &qty,
      const boost::optional<Price> &price,
      const TimeInForce &) {
    OnNewOrder(operationIdSuboperationId, QString::fromStdString(id.GetValue()),
               time, security, currency, tradingSystem, side, qty, price,
               ORDER_STATUS_SENT, QString());
  }

  void OnOperationOrderSubmitError(
      const boost::optional<std::pair<ids::uuid, int64_t>>
          &operationIdSuboperationId,
      const pt::ptime &time,
      const Security &security,
      const Currency &currency,
      const TradingSystem &tradingSystem,
      const OrderSide &side,
      const Qty &qty,
      const boost::optional<Price> &price,
      const TimeInForce &,
      const QString &error) {
    auto fakeId = QUuid::createUuid().toString();
    OnNewOrder(operationIdSuboperationId, std::move(fakeId), time, security,
               currency, tradingSystem, side, qty, price, ORDER_STATUS_ERROR,
               error);
  }

  void OnOrderUpdate(const OrderId &id,
                     const TradingSystem *tradingSystem,
                     const pt::ptime &time,
                     const OrderStatus &status,
                     const Qty &remainingQty) {
    qx::QxSqlQuery query(
        "WHERE remote_id = :id AND trading_system = :tradingSystem");
    query.bind(":id", QString::fromStdString(id.GetValue()));
    query.bind(":tradingSystem",
               QString::fromStdString(tradingSystem->GetInstanceName()));
    std::vector<Orm::Order> records;
    if (db::fetch_by_query_with_relation("Operation", query, records, m_db)
            .isValid()) {
      Assert(false);
      return;
    } else if (records.size() != 1) {
      AssertEq(1, records.size());
      return;
    }
    auto &order = records.front();
    order.setUpdateTime(ConvertToDbDateTime(time));
    order.setStatus(status);
    order.setRemainingQty(remainingQty);
    if (db::update(order, m_db).isValid()) {
      Assert(false);
      return;
    }
    emit m_self.OrderUpdate(order);
  }

  void OnOperationStart(const ids::uuid &id,
                        const pt::ptime &time,
                        const Strategy *strategySource) {
    auto operation = boost::make_shared<Orm::Operation>(ConvertToQUuid(id));
    operation->setStartTime(ConvertToDbDateTime(time));
    {
      auto strategyOrm = boost::make_shared<Orm::Strategy>(
          ConvertToQUuid(strategySource->GetId()));
      db::fetch_by_id(strategyOrm, m_db);
      strategyOrm->setName(
          QString::fromStdString(strategySource->GetInstanceName()));
      db::save(strategyOrm);
      operation->setStrategy(strategyOrm);
    }
    db::insert(operation, m_db);
    emit m_self.OperationUpdate(*operation);
  }

  void OnOperationUpdate(const ids::uuid &id, const Pnl::Data &pnl) {
    auto operation = boost::make_shared<Orm::Operation>(ConvertToQUuid(id));
    db::fetch_by_id_with_relation("Pnl", operation, m_db);
    UpdatePnl(operation, pnl);
    db::update_with_relation("Pnl", operation, m_db);
    emit m_self.OperationUpdate(*operation);
  }

  void OnOperationEnd(const ids::uuid &id,
                      const pt::ptime &time,
                      const boost::shared_ptr<const Pnl> &pnl) {
    auto operation = boost::make_shared<Orm::Operation>(ConvertToQUuid(id));
    db::fetch_by_id_with_relation("Pnl", operation, m_db);
    operation->setEndTime(ConvertToDbDateTime(time));
    operation->setStatus(CovertToOperationStatus(pnl->GetResult()));
    UpdatePnl(operation, pnl->GetData());
    db::update_with_relation("Pnl", operation, m_db);
    emit m_self.OperationUpdate(*operation);
  }

  void UpdatePnl(const boost::shared_ptr<Orm::Operation> &operation,
                 const Pnl::Data &pnlSource) {
    boost::unordered_map<std::string, boost::shared_ptr<Orm::Pnl>> index;
    for (const auto &pnl : operation->getPnl()) {
      const auto &symbol = pnl->getSymbol().toStdString();
      const auto &source = pnlSource.find(symbol);
      if (source == pnlSource.cend()) {
        db::delete_by_id(pnl, m_db);
      } else {
        index.emplace(std::move(symbol), pnl);
      }
    }

    std::vector<boost::shared_ptr<Orm::Pnl>> result;
    for (const auto &pnl : pnlSource) {
      const auto &it = index.find(pnl.first);
      boost::shared_ptr<Orm::Pnl> record;
      if (it == index.cend()) {
        record = boost::make_shared<Orm::Pnl>();
        record->setSymbol(QString::fromStdString(pnl.first));
        record->setOperation(operation);
      } else {
        record = it->second;
      }
      record->setFinancialResult(pnl.second.financialResult);
      record->setCommission(pnl.second.commission);
      result.emplace_back(record);
    }

    operation->setPnl(std::move(result));
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
      operation->OnNewPositionStart(*static_cast<Position *>(nullptr));
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

front::Engine::Engine(const fs::path &path, QWidget *parent)
    : QObject(parent),
      m_pimpl(boost::make_unique<Implementation>(*this, path)) {
  m_pimpl->InitDb();
  m_pimpl->ConnectSignals();
}

front::Engine::~Engine() {
  // Fixes second stop by StateChanged-signal.
  m_pimpl->m_engine.reset();
}

const fs::path &front::Engine::GetConfigFilePath() const {
  return m_pimpl->m_configFilePath;
}

bool front::Engine::IsStarted() const {
  return m_pimpl->m_engine ? true : false;
}

void front::Engine::Start(
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

void front::Engine::Stop() {
  if (!m_pimpl->m_engine) {
    throw Exception(tr("Engine is not started").toLocal8Bit().constData());
  }
  m_pimpl->m_engine.reset();
}

Context &front::Engine::GetContext() {
  if (!m_pimpl->m_engine) {
    throw Exception(tr("Engine is not started").toLocal8Bit().constData());
  }
  return m_pimpl->m_engine->GetContext();
}

const front::DropCopy &front::Engine::GetDropCopy() const {
  return m_pimpl->m_dropCopy;
}

RiskControlScope &front::Engine::GetRiskControl(const TradingMode &mode) {
  return *m_pimpl->m_riskControls[mode];
}

std::vector<boost::shared_ptr<Orm::Operation>> front::Engine::GetOperations(
    bool isTradesIncluded,
    bool isErrorsIncluded,
    bool isCancelsIncluded,
    const QDate &dateFrom,
    const QDate &dateTo) const {
  std::vector<boost::shared_ptr<Orm::Operation>> result;
  QString querySql = "WHERE startTime >= :timeFrom AND endTime < :timeTo";
  if (!isTradesIncluded || !isErrorsIncluded || !isCancelsIncluded) {
    QList<QString> list;
    if (!isTradesIncluded) {
      list.append(QString::number(Orm::OperationStatus::LOSS));
      list.append(QString::number(Orm::OperationStatus::PROFIT));
    }
    if (!isErrorsIncluded) {
      list.append(QString::number(Orm::OperationStatus::ERROR));
    }
    if (!isCancelsIncluded) {
      list.append(QString::number(Orm::OperationStatus::CANCELED));
    }
    querySql += " AND t_Operation.status NOT IN (" + list.join(", ") + ')';
  }
  qx::QxSqlQuery query(querySql);
  query.bind(":timeFrom", QDateTime(dateFrom, QTime(0, 0)));
  query.bind(":timeTo", QDateTime(dateTo, QTime(0, 0)).addDays(1));

  Verify(!db::fetch_by_query_with_all_relation(query, result, m_pimpl->m_db)
              .isValid());
  return result;
}

#ifdef DEV_VER
void front::Engine::Test() { m_pimpl->Test(); }
#endif

void front::Engine::StoreConfig(const Strategy &strategy,
                                QString &&config,
                                bool isActive) {
  auto strategyOrm =
      boost::make_shared<Orm::Strategy>(ConvertToQUuid(strategy.GetId()));
  db::fetch_by_id(strategyOrm, m_pimpl->m_db);
  strategyOrm->setConfig(std::move(config));
  strategyOrm->setIsActive(isActive);
  db::save(strategyOrm);
}

void front::Engine::ForEachActiveStrategy(
    const boost::function<void(const QUuid &id, const QString &config)>
        &callback) const {
  qx::QxSqlQuery query("WHERE IsActive != 0");
  std::vector<boost::shared_ptr<Orm::Strategy>> result;
  Verify(!db::fetch_by_query_with_all_relation(query, result, m_pimpl->m_db)
              .isValid());
  for (const auto &strategy : result) {
    callback(strategy->getId(), strategy->getConfig());
  }
}
