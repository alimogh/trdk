//
//    Created: 2018/11/14 15:26
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "TradingSystem.hpp"
#include "Request.hpp"
#include "Session.hpp"
#include "TradingSystemConnection.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace Interaction;
using namespace Rest;
using namespace Poloniex;
namespace ptr = boost::property_tree;
namespace p = Poloniex;

boost::shared_ptr<trdk::TradingSystem> CreateTradingSystem(
    const TradingMode &mode,
    Context &context,
    std::string instanceName,
    std::string title,
    const ptr::ptree &config) {
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  const auto &result = boost::make_shared<p::TradingSystem>(
      App::GetInstance(), mode, context, std::move(instanceName),
      std::move(title), config);
  return result;
}

p::TradingSystem::TradingSystem(const App &,
                                const TradingMode &mode,
                                Context &context,
                                std::string instanceName,
                                std::string title,
                                const ptr::ptree &conf)
    : Base(mode, context, std::move(instanceName), std::move(title)),
      m_settings(conf, GetLog()),
      m_products(GetProductList()),
      m_nonces(
          std::make_unique<NonceStorage::UnsignedInt64MicrosecondsGenerator>()),
      m_balances(*this, GetLog(), GetTradingLog()),
      m_session(CreateSession(m_settings, true)) {}

p::TradingSystem::~TradingSystem() {
  try {
    boost::mutex::scoped_lock lock(m_listeningConnectionMutex);
    const auto connection = std::move(m_listeningConnection);
    lock.unlock();
    UseUnused(connection);
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

bool p::TradingSystem::IsConnected() const { return m_isConnected; }

Volume p::TradingSystem::CalcCommission(const Qty &qty,
                                        const Price &price,
                                        const OrderSide &,
                                        const trdk::Security &) const {
  return (qty * price) * (0.2 / 100);
}

void p::TradingSystem::CreateConnection() {
  GetLog().Debug("Creating connection...");

  Assert(!m_isConnected);

  try {
    {
      PrivateRequest balancesRequest("returnBalances", "", GetContext(),
                                     m_settings, m_nonces, GetLog(),
                                     GetTradingLog());
      const auto response = boost::get<1>(balancesRequest.Send(m_session));
      for (const auto &node : response) {
        m_balances.Set(node.first, node.second.get_value<Volume>(), 0);
      }
    }

    const boost::mutex::scoped_lock lock(m_listeningConnectionMutex);
    auto listeningConnection = CreateListeningConnection();

    m_listeningConnection = std::move(listeningConnection);

  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }

  m_isConnected = true;
}

std::unique_ptr<OrderTransactionContext> p::TradingSystem::SendOrderTransaction(
    trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const boost::optional<Price> &price,
    const OrderParams &params,
    const OrderSide &side,
    const TimeInForce &tif) {
  static_assert(numberOfTimeInForces == 5, "List changed.");
  switch (tif) {
    case TIME_IN_FORCE_IOC:
      return SendOrderTransactionAndEmulateIoc(security, currency, qty, price,
                                               params, side);
    case TIME_IN_FORCE_GTC:
      break;
    default:
      throw Exception("Order time-in-force type is not supported");
  }
  if (currency != security.GetSymbol().GetCurrency()) {
    throw Exception("Trading system supports only security quote currency");
  }
  if (!price) {
    throw Exception("Market order is not supported");
  }

  throw MethodIsNotImplementedException("");
}

void p::TradingSystem::SendCancelOrderTransaction(
    const OrderTransactionContext &) {
  throw MethodIsNotImplementedException("");
}

Balances &p::TradingSystem::GetBalancesStorage() { return m_balances; }

boost::optional<OrderCheckError> p::TradingSystem::CheckOrder(
    const trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const boost::optional<Price> &price,
    const OrderSide &side) const {
  auto result = Base::CheckOrder(security, currency, qty, price, side);
  if (result) {
    return result;
  }
  return OrderCheckError{0, 0, 0};
}

bool p::TradingSystem::CheckSymbol(const std::string &symbol) const {
  return Base::CheckSymbol(symbol) && m_products.count(symbol) > 0;
}

boost::shared_ptr<TradingSystemConnection>
p::TradingSystem::CreateListeningConnection() {
  auto result = boost::make_shared<TradingSystemConnection>();
  result->Connect();
  result->Start(
      m_settings, m_nonces,
      TradingSystemConnection::Events{
          []() -> const TradingSystemConnection::EventInfo { return {}; },
          [this](const TradingSystemConnection::EventInfo &,
                 const ptr::ptree &message) { HandleMessage(message); },
          [this]() {
            const boost::mutex::scoped_lock lock(m_listeningConnectionMutex);
            if (!m_listeningConnection) {
              GetLog().Debug("Disconnected.");
              return;
            }
            const auto connection = std::move(m_listeningConnection);
            GetLog().Warn("Connection lost.");
            GetContext().GetTimer().Schedule(
                [this, connection]() {
                  {
                    const boost::mutex::scoped_lock lock(
                        m_listeningConnectionMutex);
                  }
                  ScheduleListeningConnectionReconnect();
                },
                m_timerScope);
          },
          [this](const std::string &event) { GetLog().Debug(event.c_str()); },
          [this](const std::string &event) { GetLog().Info(event.c_str()); },
          [this](const std::string &event) { GetLog().Warn(event.c_str()); },
          [this](const std::string &event) { GetLog().Error(event.c_str()); }});
  return result;
}

void p::TradingSystem::ScheduleListeningConnectionReconnect() {
  GetContext().GetTimer().Schedule(
      [this]() {
        const boost::mutex::scoped_lock lock(m_listeningConnectionMutex);
        GetLog().Info("Reconnecting...");
        Assert(!m_listeningConnection);
        try {
          m_listeningConnection = CreateListeningConnection();
        } catch (const std::exception &ex) {
          GetLog().Error("Failed to connect: \"%1%\".", ex.what());
          ScheduleListeningConnectionReconnect();
        }
      },
      m_timerScope);
}

void p::TradingSystem::HandleMessage(const ptr::ptree &message) {
  auto hasChannelId = false;
  auto seqNumber = false;
  for (const auto &node : message) {
    if (!hasChannelId) {
      const auto &channel = node.second.get_value<int>();
      if (channel == 1010) {
        return;
      }
      if (channel != 1000) {
        throw Exception("Message from unknown channel");
      }
      hasChannelId = true;
    } else if (!seqNumber) {
      seqNumber = true;
    } else {
      for (const auto &update : node.second) {
        for (const auto &field : update.second) {
          switch (field.second.get_value<char>()) {
            case 'b':
              UpdateBalance(update.second);
              break;
            default:
              break;
          }
        }
      }
    }
  }
}

void p::TradingSystem::UpdateBalance(const ptr::ptree &message) {
  auto hasMessageType = false;
  auto hasUpdateType = false;
  boost::optional<uintmax_t> currencyId;
  for (const auto &fieldNode : message) {
    const auto &field = fieldNode.second;
    if (!hasMessageType) {
      hasMessageType = true;
    } else if (!currencyId) {
      currencyId.emplace(field.get_value<uintmax_t>());
    } else if (!hasUpdateType) {
      if (field.get_value<char>() != 'e') {
        return;
      }
      hasUpdateType = true;
    } else {
      m_balances.Modify(
          ResolveCurrency(*currencyId),
          [field](const bool isNew, const Volume &available, const Volume &)
              -> boost::optional<std::pair<Volume, Volume>> {
            Assert(isNew);
            if (!isNew) {
              return boost::none;
            }
            return std::make_pair(available + field.get_value<Volume>(), 0);
          });
    }
  }
}
