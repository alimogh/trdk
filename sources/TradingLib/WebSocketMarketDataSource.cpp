/*******************************************************************************
 *   Created: 2018/04/07 14:21:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "WebSocketMarketDataSource.hpp"
#include "WebSocketConnection.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace TradingLib;
namespace ptr = boost::property_tree;

class WebSocketMarketDataSource::Implementation {
 public:
  WebSocketMarketDataSource &m_self;

  boost::mutex m_connectionMutex;
  std::unique_ptr<WebSocketConnection> m_connection;

  Timer::Scope m_timerScope;

  explicit Implementation(WebSocketMarketDataSource &self) : m_self(self) {}

  void StartConnection(Connection &connection) {
    connection.StartData(Connection::Events{
        [this]() -> Connection::EventInfo {
          const auto &context = m_self.GetContext();
          return {context.GetCurrentTime(),
                  context.StartStrategyTimeMeasurement()};
        },
        [this](const Connection::EventInfo &info, const ptr::ptree &message) {
          m_self.HandleMessage(info.readTime, message, info.delayMeasurement);
        },
        [this]() {
          const boost::mutex::scoped_lock lock(m_connectionMutex);
          if (!m_connection) {
            m_self.GetLog().Debug("Disconnected.");
            return;
          }
          const boost::shared_ptr<WebSocketConnection> connection(
              std::move(m_connection));
          m_self.GetLog().Warn("Connection lost.");
          m_self.GetContext().GetTimer().Schedule(
              [this, connection]() {
                { const boost::mutex::scoped_lock lock(m_connectionMutex); }
                ScheduleReconnect();
              },
              m_timerScope);
        },
        [this](const std::string &event) {
          m_self.GetLog().Debug(event.c_str());
        },
        [this](const std::string &event) {
          m_self.GetLog().Info(event.c_str());
        },
        [this](const std::string &event) {
          m_self.GetLog().Warn(event.c_str());
        },
        [this](const std::string &event) {
          m_self.GetLog().Error(event.c_str());
        }});
  }

  void ScheduleReconnect() {
    m_self.GetContext().GetTimer().Schedule(
        [this]() {
          m_self.GetLog().Info("Reconnecting...");
          Assert(!m_connection);
          auto connection = m_self.CreateConnection();
          try {
            connection->Connect();
            StartConnection(*connection);
          } catch (const CommunicationError &ex) {
            m_self.GetLog().Error("Failed to connect: \"%1%\".", ex.what());
            ScheduleReconnect();
            return;
          }
          const boost::mutex::scoped_lock lock(m_connectionMutex);
          Assert(!m_connection);
          m_connection = std::move(connection);
        },
        m_timerScope);
  }
};

WebSocketMarketDataSource::WebSocketMarketDataSource(Context &context,
                                                     std::string instanceName,
                                                     std::string title)
    : MarketDataSource(context, std::move(instanceName), std::move(title)),
      m_pimpl(boost::make_unique<Implementation>(*this)) {}

WebSocketMarketDataSource::WebSocketMarketDataSource(
    WebSocketMarketDataSource &&) noexcept = default;

WebSocketMarketDataSource::~WebSocketMarketDataSource() {
  {
    boost::mutex::scoped_lock lock(m_pimpl->m_connectionMutex);
    const auto connection = std::move(m_pimpl->m_connection);
    lock.unlock();
  }
  // Each object, that implements CreateNewSecurityObject should wait for log
  // flushing before destroying objects:
  GetTradingLog().WaitForFlush();
}

void WebSocketMarketDataSource::Connect() {
  Assert(!m_pimpl->m_connection);
  auto connection = CreateConnection();
  connection->Connect();
  m_pimpl->m_connection = std::move(connection);
}

void WebSocketMarketDataSource::SubscribeToSecurities() {
  boost::mutex::scoped_lock lock(m_pimpl->m_connectionMutex);
  Assert(m_pimpl->m_connection);
  auto connection = std::move(m_pimpl->m_connection);
  m_pimpl->ScheduleReconnect();
  lock.unlock();
}
