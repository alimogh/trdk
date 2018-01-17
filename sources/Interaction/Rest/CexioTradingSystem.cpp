/*******************************************************************************
 *   Created: 2018/01/18 01:27:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "CexioTradingSystem.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::Crypto;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

CexioTradingSystem::Settings::Settings(const IniSectionRef &conf,
                                       ModuleEventsLog &log)
    : Rest::Settings(conf, log),
      apiKey(conf.ReadKey("api_key")),
      apiSecret(conf.ReadKey("api_secret")) {
  log.Info("API key: \"%1%\". API secret: %2%.",
           apiKey,                                     // 1
           apiSecret.empty() ? "not set" : "is set");  // 2
}

////////////////////////////////////////////////////////////////////////////////

CexioTradingSystem::CexioTradingSystem(const App &,
                                       const TradingMode &mode,
                                       Context &context,
                                       const std::string &instanceName,
                                       const IniSectionRef &conf)
    : Base(mode, context, instanceName),
      m_settings(conf, GetLog()),
      m_balances(GetLog(), GetTradingLog()),
      // m_balancesRequest(m_settings, GetContext(), GetLog()),
      m_tradingSession(CreateSession("cex.io", m_settings, true)),
      m_pollingSession(CreateSession("cex.io", m_settings, false)),
      m_pollingTask(m_settings.pollingSetttings, GetLog()) {}

void CexioTradingSystem::CreateConnection(const IniSectionRef &) {
  Assert(m_products.empty());

  try {
    UpdateBalances();
    m_products =
        RequestCexioProductList(*m_tradingSession, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }

  m_pollingTask.AddTask(
      "Orders", 0,
      [this]() {
        UpdateOrders();
        return true;
      },
      m_settings.pollingSetttings.GetActualOrdersRequestFrequency(), true);
  m_pollingTask.AddTask(
      "Balances", 1,
      [this]() {
        UpdateBalances();
        return true;
      },
      m_settings.pollingSetttings.GetBalancesRequestFrequency(), true);

  m_pollingTask.AccelerateNextPolling();
}

Volume CexioTradingSystem::CalcCommission(const Volume &volume,
                                          const trdk::Security &) const {
  return volume * (25 / 100);
}

void CexioTradingSystem::UpdateBalances() {}

void CexioTradingSystem::UpdateOrders() {}

boost::optional<CexioTradingSystem::OrderCheckError>
CexioTradingSystem::CheckOrder(const trdk::Security &security,
                               const Currency &currency,
                               const Qty &qty,
                               const boost::optional<Price> &price,
                               const OrderSide &side) const {
  {
    const auto result = Base::CheckOrder(security, currency, qty, price, side);
    if (result) {
      return result;
    }
  }

  return boost::none;
}

std::unique_ptr<OrderTransactionContext>
CexioTradingSystem::SendOrderTransaction(trdk::Security &security,
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

  const auto &product = m_products.find(security.GetSymbol().GetSymbol());
  if (product == m_products.cend()) {
    throw Exception("Symbol is not supported by exchange");
  }

  throw MethodIsNotImplementedException("REST-module error");
}

void CexioTradingSystem::SendCancelOrderTransaction(
    const OrderTransactionContext &) {
  throw MethodIsNotImplementedException("REST-module error");
}

void CexioTradingSystem::OnTransactionSent(
    const OrderTransactionContext &transaction) {
  Base::OnTransactionSent(transaction);
  m_pollingTask.AccelerateNextPolling();
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::TradingSystem> CreateCexioTradingSystem(
    const TradingMode &mode,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<CexioTradingSystem>(
      App::GetInstance(), mode, context, instanceName, configuration);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
