//
//    Created: 2018/09/19 9:16 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "BittrexAccount.hpp"
#include "BittrexRequest.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction::Rest;
namespace ptr = boost::property_tree;
namespace net = Poco::Net;

namespace {
class WithdrawRequest : public BittrexPrivateRequest {
 public:
  explicit WithdrawRequest(const std::string &productId,
                           const Qty &qty,
                           const std::string &address,
                           const BittrexSettings &settings,
                           const Context &context,
                           ModuleEventsLog &log,
                           ModuleTradingLog &tradingLog)
      : BittrexPrivateRequest("withdraw",
                              CreateUriParams(productId, qty, address),
                              settings,
                              context,
                              log,
                              &tradingLog) {}

  bool IsPriority() const override { return true; }

  OrderId SendOrderTransaction(
      std::unique_ptr<net::HTTPSClientSession> &session) {
    const auto response = boost::get<1>(Base::Send(session));
    try {
      return response.get<std::string>("uuid");
    } catch (const ptr::ptree_error &ex) {
      boost::format error(
          R"(Wrong server response to the request "%1%" (%2%): "%3%")");
      error % GetName()            // 1
          % GetRequest().getURI()  // 2
          % ex.what();             // 3
      throw Exception(error.str().c_str());
    }
  }

 private:
  static std::string CreateUriParams(const std::string &productId,
                                     const Qty &qty,
                                     const std::string &address) {
    boost::format result("currency=%1%&quantity=%2%&address=%3%");
    result % productId  // 1
        % qty           // 2
        % address;      // 3
    return result.str();
  }
};

}  // namespace

BittrexAccount::BittrexAccount(
    const boost::unordered_map<std::string, BittrexProduct> &products,
    std::unique_ptr<Poco::Net::HTTPSClientSession> &session,
    const BittrexSettings &settings,
    const Context &context,
    ModuleEventsLog &eventsLog,
    ModuleTradingLog &tradingLog)
    : m_products(products),
      m_session(session),
      m_settings(settings),
      m_context(context),
      m_log(eventsLog),
      m_tradingLog(tradingLog) {}

bool BittrexAccount::IsWithdrawsFunds() const { return true; }

void BittrexAccount::WithdrawFunds(const std::string &symbol,
                                   const Volume &volume,
                                   const std::string &targetInfo) {
  const auto &product = m_products.find(symbol);
  if (product == m_products.cend()) {
    throw Exception("Symbol is not supported by exchange");
  }
  WithdrawRequest(product->second.id, volume, targetInfo, m_settings, m_context,
                  m_log, m_tradingLog)
      .SendOrderTransaction(m_session);
}