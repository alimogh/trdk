/**************************************************************************
 *   Created: May 26, 2012 9:44:52 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "IbTradingSystem.hpp"
#include "Core/Security.hpp"
#include "IbClient.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::InteractiveBrokers;

namespace ib = trdk::Interaction::InteractiveBrokers;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

ib::TradingSystem::TradingSystem(const TradingMode &mode,
                                 size_t tradingSystemIndex,
                                 size_t marketDataSourceIndex,
                                 Context &context,
                                 const std::string &instanceName,
                                 const Lib::IniSectionRef &conf)
    : trdk::TradingSystem(mode, tradingSystemIndex, context, instanceName),
      trdk::MarketDataSource(marketDataSourceIndex, context, instanceName),
      m_isTestSource(conf.ReadBoolKey("test_source", false)) {}

ib::TradingSystem::~TradingSystem() {}

void ib::TradingSystem::Connect(const IniSectionRef &settings) {
  // Implementation for trdk::MarketDataSource.
  if (IsConnected()) {
    return;
  }
  GetMdsLog().Info("Creating connection...");
  CreateConnection(settings);
}

bool ib::TradingSystem::IsConnected() const { return m_client ? true : false; }

void ib::TradingSystem::CreateConnection(const IniSectionRef &settings) {
  Assert(!m_client);
  Assert(m_securities.empty());

  std::unique_ptr<Account> account;
  std::unique_ptr<Client> client(
      new Client(*this, settings.ReadBoolKey("no_history", false),
                 settings.ReadTypedKey<int>("client_id", 0),
                 settings.ReadKey("ip_address", "127.0.0.1"),
                 settings.ReadTypedKey<unsigned short>("port", 7497)));

  if (settings.IsKeyExist("account")) {
    account.reset(new Account);
    client->SetAccount(settings.ReadKey("account", ""), *account);
  }

  client->Subscribe(
      boost::bind(&TradingSystem::OnOrderStatus, this, _1, _2, _3, _4, _5, _6,
                  _7),
      boost::bind(&TradingSystem::OnExecution, this, _1, _2),
      boost::bind(&TradingSystem::OnCommissionReport, this, _1, _2, _3));

  client->Start();

  client.swap(m_client);
  account.swap(m_account);
}

void ib::TradingSystem::SubscribeToSecurities() {
  Assert(m_client);
  if (!m_client) {
    return;
  }
  while (!m_unsubscribedSecurities.empty()) {
    auto security = m_unsubscribedSecurities.front();
    m_unsubscribedSecurities.pop_front();
    m_securities.emplace_back(security);
    m_client->SubscribeToMarketData(*security);
  }
}

const ib::TradingSystem::Account &ib::TradingSystem::GetAccount() const {
  if (!m_account) {
    throw UnknownAccountError("Account not specified");
  }
  return *m_account;
}

trdk::Security &ib::TradingSystem::CreateNewSecurityObject(
    const Symbol &symbol) {
  const auto &result = boost::make_shared<ib::Security>(GetContext(), symbol,
                                                        *this, m_isTestSource);
  result->SetTradingSessionState(pt::not_a_date_time, true);
  m_unsubscribedSecurities.emplace_back(result);
  return *result;
}

void ib::TradingSystem::SwitchToContract(
    trdk::Security &security, const ContractExpiration &&expiration) const {
  m_client->SwitchToContract(
      *boost::polymorphic_downcast<ib::Security *>(&security),
      std::move(expiration));
}

boost::optional<ContractExpiration> ib::TradingSystem::FindContractExpiration(
    const Symbol &symbol, const gr::date &date) const {
  boost::optional<ContractExpiration> result;
  for (const ContractDetails &contract :
       m_client->MatchContractDetails(symbol)) {
    const ContractExpiration expiration(
        Lib::ConvertToDateFromYyyyMmDd(contract.summary.expiry));
    if (date > expiration.GetDate()) {
      continue;
    }
    if (!result || *result > expiration) {
      result = std::move(expiration);
    }
  }
  Assert(!result || date <= result->GetDate());
  return result;
}

void ib::TradingSystem::SendCancelOrder(const trdk::OrderId &orderId) {
  m_client->CancelOrder(orderId);
}

trdk::OrderId ib::TradingSystem::SendSellAtMarketPrice(
    trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const OrderParams &params,
    const OrderStatusUpdateSlot &statusUpdateSlot) {
  AssertEq(security.GetSymbol().GetCurrency(), currency);
  UseUnused(currency);
  PlacedOrder order = {};
  order.id = m_client->PlaceSellOrder(security, qty, params);
  order.security = &security;
  order.callback = statusUpdateSlot;
  return RegOrder(std::move(order));
}

trdk::OrderId ib::TradingSystem::SendSell(
    trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    const OrderStatusUpdateSlot &&statusUpdateSlot) {
  AssertEq(security.GetSymbol().GetCurrency(), currency);
  UseUnused(currency);
  PlacedOrder order = {};
  order.id = m_client->PlaceSellOrder(security, qty, price, params);
  order.security = &security;
  order.callback = std::move(statusUpdateSlot);
  return RegOrder(std::move(order));
}

trdk::OrderId ib::TradingSystem::SendSellAtMarketPriceWithStopPrice(
    trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const Price &stopPrice,
    const OrderParams &params,
    const OrderStatusUpdateSlot &statusUpdateSlot) {
  AssertEq(security.GetSymbol().GetCurrency(), currency);
  UseUnused(currency);
  PlacedOrder order = {};
  order.id =
      m_client->PlaceSellOrderWithMarketPrice(security, qty, stopPrice, params);
  order.security = &security;
  order.callback = statusUpdateSlot;
  return RegOrder(std::move(order));
}

trdk::OrderId ib::TradingSystem::SendSellImmediatelyOrCancel(
    trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    const OrderStatusUpdateSlot &statusUpdateSlot) {
  AssertEq(security.GetSymbol().GetCurrency(), currency);
  UseUnused(currency);
  return RegOrder(
      PlacedOrder{m_client->PlaceSellIocOrder(security, qty, price, params),
                  &security, statusUpdateSlot});
}

trdk::OrderId ib::TradingSystem::SendBuyAtMarketPrice(
    trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const OrderParams &params,
    const OrderStatusUpdateSlot &statusUpdateSlot) {
  AssertEq(security.GetSymbol().GetCurrency(), currency);
  UseUnused(currency);
  PlacedOrder order = {};
  order.id = m_client->PlaceBuyOrder(security, qty, params);
  order.security = &security;
  order.callback = statusUpdateSlot;
  return RegOrder(std::move(order));
}

trdk::OrderId ib::TradingSystem::SendSellAtMarketPriceImmediatelyOrCancel(
    trdk::Security &,
    const trdk::Lib::Currency &,
    const trdk::Qty &,
    const trdk::OrderParams &,
    const OrderStatusUpdateSlot &) {
  throw MethodDoesNotImplementedError("Method is not implemented");
}

trdk::OrderId ib::TradingSystem::SendBuy(
    trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    const OrderStatusUpdateSlot &&statusUpdateSlot) {
  AssertEq(security.GetSymbol().GetCurrency(), currency);
  UseUnused(currency);
  PlacedOrder order = {};
  order.id = m_client->PlaceBuyOrder(security, qty, price, params);
  order.security = &security;
  order.callback = std::move(statusUpdateSlot);
  return RegOrder(std::move(order));
}

trdk::OrderId ib::TradingSystem::SendBuyAtMarketPriceWithStopPrice(
    trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const Price &stopPrice,
    const OrderParams &params,
    const OrderStatusUpdateSlot &statusUpdateSlot) {
  AssertEq(security.GetSymbol().GetCurrency(), currency);
  UseUnused(currency);
  PlacedOrder order = {};
  order.id =
      m_client->PlaceBuyOrderWithMarketPrice(security, qty, stopPrice, params);
  order.security = &security;
  order.callback = statusUpdateSlot;
  return RegOrder(std::move(order));
}

trdk::OrderId ib::TradingSystem::SendBuyImmediatelyOrCancel(
    trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    const OrderStatusUpdateSlot &statusUpdateSlot) {
  AssertEq(security.GetSymbol().GetCurrency(), currency);
  UseUnused(currency);
  return RegOrder(
      PlacedOrder{m_client->PlaceBuyIocOrder(security, qty, price, params),
                  &security, statusUpdateSlot});
}

trdk::OrderId ib::TradingSystem::SendBuyAtMarketPriceImmediatelyOrCancel(
    trdk::Security &,
    const trdk::Lib::Currency &,
    const trdk::Qty &,
    const trdk::OrderParams &,
    const OrderStatusUpdateSlot &) {
  throw MethodDoesNotImplementedError("Method is not implemented");
}

trdk::OrderId ib::TradingSystem::RegOrder(PlacedOrder &&order) {
  const OrdersWriteLock lock(m_ordersMutex);
  Assert(m_placedOrders.get<ByOrder>().find(order.id) ==
         m_placedOrders.get<ByOrder>().cend());
  const auto id = order.id;
  m_placedOrders.emplace(std::move(order));
  return id;
}

void ib::TradingSystem::OnOrderStatus(const trdk::OrderId &id,
                                      int permOrderId,
                                      const OrderStatus &status,
                                      const Qty &filled,
                                      const Qty &remaining,
                                      double lastFillPrice,
                                      OrderCallbackList &callBackList) {
  TradeInfo tradeData = {};
  OrderStatusUpdateSlot callBack;
  boost::optional<Volume> commission;
  {
    auto &index = m_placedOrders.get<ByOrder>();
    const OrdersWriteLock lock(m_ordersMutex);
    const auto pos = index.find(id);
    if (pos == index.cend()) {
      /*GetTsLog().Debug(
          "Failed to find order by ID \"%1%\""
          " (status %2%, filled %3%, remaining %4%"
          ", last price %5%). ",
          id, status, filled, remaining, lastFillPrice);*/
      return;
    }
    callBack = pos->callback;
    switch (status) {
      default:
        return;
      case ORDER_STATUS_FILLED:
        AssertLt(0, filled);
        AssertGt(filled, pos->filled);
        if (remaining == 0 && pos->HasUnreceviedCommission()) {
          const_cast<PlacedOrder &>(*pos).finalUpdate =
              PlacedOrder::FinalUpdate{permOrderId, status, filled, remaining,
                                       lastFillPrice};
          return;
        }
        tradeData.price = lastFillPrice;
        AssertLt(0, tradeData.price);
        tradeData.qty = filled - pos->filled;
        AssertLt(0, tradeData.qty);
        pos->UpdateFilled(filled);
        if (callBack) {
          commission = pos->CalcCommission();
        }
        if (remaining == 0) {
          index.erase(pos);
          m_executionSet.get<ByOrder>().erase(id);
        }
        break;
      case ORDER_STATUS_CANCELLED:
      case ORDER_STATUS_INACTIVE:
      case ORDER_STATUS_ERROR:
        if (pos->HasUnreceviedCommission()) {
          const_cast<PlacedOrder &>(*pos).finalUpdate =
              PlacedOrder::FinalUpdate{permOrderId, status, filled, remaining,
                                       lastFillPrice};
          return;
        }
        if (filled > pos->filled) {
          TradeInfo iocTradeData = {};
          AssertGt(filled, pos->filled);
          iocTradeData.price = lastFillPrice;
          AssertLt(0, iocTradeData.price);
          iocTradeData.qty = filled - pos->filled;
          AssertLt(0, iocTradeData.qty);
          pos->UpdateFilled(filled);
          if (callBack) {
            callBackList.emplace_back(
                [callBack, id, permOrderId, remaining, iocTradeData]() {
                  callBack(id, boost::lexical_cast<std::string>(permOrderId),
                           ORDER_STATUS_FILLED_PARTIALLY, remaining,
                           boost::none, &iocTradeData);
                });
          }
        }
        if (callBack) {
          commission = pos->CalcCommission();
        }
        index.erase(pos);
        m_executionSet.get<ByOrder>().erase(id);
        break;
    }
  }
  if (!callBack) {
    return;
  }
  callBackList.emplace_back(
      [callBack, id, permOrderId, status, remaining, commission, tradeData]() {
        const TradeInfo *const tradeDataPtr =
            tradeData.qty != 0 ? &tradeData : nullptr;
        callBack(id, boost::lexical_cast<std::string>(permOrderId), status,
                 remaining, commission, tradeDataPtr);
      });
}

void ib::TradingSystem::OnExecution(const trdk::OrderId &orderId,
                                    const std::string &execId) {
  auto &orderIndex = m_placedOrders.get<ByOrder>();
  const OrdersWriteLock lock(m_ordersMutex);
  auto orderIt = orderIndex.find(orderId);
  if (orderIt == orderIndex.cend()) {
    return;
  }
  Verify(m_executionSet.emplace(Execution{execId, orderId}).second);
  for (auto &commission : const_cast<PlacedOrder &>(*orderIt).commissions) {
    AssertNe(execId, commission.execId);
    if (execId == commission.execId) {
      Assert(!commission.commission);
      commission.commission = boost::none;
      return;
    }
  }
  const_cast<PlacedOrder &>(*orderIt).commissions.emplace_back(
      PlacedOrder::CommissionRecord{execId});
}

void ib::TradingSystem::OnCommissionReport(const std::string &execId,
                                           double commissionValue,
                                           OrderCallbackList &callBackList) {
  trdk::OrderId orderId;
  PlacedOrder::FinalUpdate finalUpdate;

  {
    auto &orderIndex = m_placedOrders.get<ByOrder>();
    auto &execIndex = m_executionSet.get<ByExecution>();

    const OrdersWriteLock lock(m_ordersMutex);

    const auto &execIt = execIndex.find(execId);
    if (execIt == execIndex.cend()) {
      return;
    }

    auto orderIt = orderIndex.find(execIt->orderId);
    Assert(orderIt != orderIndex.cend());
    if (orderIt == orderIndex.cend()) {
      return;
    }

    for (auto &commission : const_cast<PlacedOrder &>(*orderIt).commissions) {
      if (execId == commission.execId) {
        Assert(!commission.commission);
        commission.commission = commissionValue;
        break;
      }
    }

    if (orderIt->HasUnreceviedCommission() || !orderIt->finalUpdate) {
      return;
    }

    orderId = execIt->orderId;
    finalUpdate = std::move(*orderIt->finalUpdate);
    const_cast<PlacedOrder &>(*orderIt).finalUpdate = boost::none;
  }

  OnOrderStatus(orderId, finalUpdate.permOrderId, finalUpdate.status,
                finalUpdate.filled, finalUpdate.remaining,
                finalUpdate.lastFillPrice, callBackList);
}
