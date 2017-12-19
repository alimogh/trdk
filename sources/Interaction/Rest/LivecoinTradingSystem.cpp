/*******************************************************************************
 *   Created: 2017/12/19 20:59:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "LivecoinTradingSystem.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::Crypto;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

LivecoinTradingSystem::Settings::Settings(const IniSectionRef &conf,
                                          ModuleEventsLog &log)
    : Rest::Settings(conf, log),
      apiKey(conf.ReadKey("api_key")),
      apiSecret(Base64::Decode(conf.ReadKey("api_secret"))) {
  log.Info("API key: \"%1%\". API secret: %2%.",
           apiKey,                                     // 1
           apiSecret.empty() ? "not set" : "is set");  // 2
}

////////////////////////////////////////////////////////////////////////////////

LivecoinTradingSystem::LivecoinTradingSystem(const App &,
                                             const TradingMode &mode,
                                             Context &context,
                                             const std::string &instanceName,
                                             const IniSectionRef &conf)
    : Base(mode, context, instanceName),
      m_settings(conf, GetLog()),
      m_balances(GetLog(), GetTradingLog()),
      m_tradingSession("api.livecoin.net"),
      m_pullingSession("api.livecoin.net"),
      m_pullingTask(m_settings.pullingSetttings, GetLog()) {}

void LivecoinTradingSystem::CreateConnection(const IniSectionRef &) {
  Assert(m_products.empty());

  try {
    //    UpdateBalances();
    m_products =
        RequestLivecoinProductList(m_tradingSession, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }

  //  m_pullingTask.AddTask(
  //      "Balances", 1,
  //      [this]() {
  //        UpdateBalances();
  //        return true;
  //      },
  //      m_settings.pullingSetttings.GetBalancesRequestFrequency());

  m_pullingTask.AccelerateNextPulling();
}

Volume LivecoinTradingSystem::CalcCommission(const Volume &volume,
                                             const trdk::Security &) const {
  return volume * (0.18 / 100);
}

boost::optional<LivecoinTradingSystem::OrderCheckError>
LivecoinTradingSystem::CheckOrder(const trdk::Security &security,
                                  const Currency &currency,
                                  const Qty &internalQty,
                                  const boost::optional<Price> &internalPrice,
                                  const OrderSide &side) const {
  {
    const auto result =
        Base::CheckOrder(security, currency, internalQty, internalPrice, side);
    if (result) {
      return result;
    }
  }

  return boost::none;
}

std::unique_ptr<OrderTransactionContext>
LivecoinTradingSystem::SendOrderTransaction(trdk::Security &security,
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
      throw TradingSystem::Error("Order time-in-force type is not supported");
  }
  if (currency != security.GetSymbol().GetCurrency()) {
    throw TradingSystem::Error(
        "Trading system supports only security quote currency");
  }
  if (!price) {
    throw TradingSystem::Error("Market order is not supported");
  }

  const auto &product = m_products.find(security.GetSymbol().GetSymbol());
  if (product == m_products.cend()) {
    throw TradingSystem::Error("Symbol is not supported by exchange");
  }

  // const auto &productId = product->second.id;

  throw Exception("Auth error");
}

void LivecoinTradingSystem::SendCancelOrderTransaction(
    const OrderId & /*orderId*/) {
  throw Exception("Auth error");
}

void LivecoinTradingSystem::OnTransactionSent(const OrderId &orderId) {
  Base::OnTransactionSent(orderId);
  m_pullingTask.AccelerateNextPulling();
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::TradingSystem> CreateLivecoinTradingSystem(
    const TradingMode &mode,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<LivecoinTradingSystem>(
      App::GetInstance(), mode, context, instanceName, configuration);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
