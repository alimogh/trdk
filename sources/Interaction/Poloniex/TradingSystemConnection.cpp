//
//    Created: 2018/11/19 14:49
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "TradingSystemConnection.hpp"
#include "AuthSettings.hpp"

using namespace trdk;
using namespace Interaction;
using namespace Rest;
using namespace Poloniex;
using namespace Lib::Crypto;
namespace ptr = boost::property_tree;

void TradingSystemConnection::Start(const AuthSettings &settings,
                                    NonceStorage &nonces,
                                    const Events &events) {
  Handshake("/");
  Base::Start(events);
  {
    ptr::ptree request;
    request.add("command", "subscribe");
    request.add("channel", "1000");
    request.add("key", settings.apiKey);
    auto nonce = nonces.TakeNonce();
    const auto &payload =
        "nonce=" + boost::lexical_cast<std::string>(nonce.Get());
    request.add("payload", payload);
    request.add("sign", EncodeToHex(Hmac::CalcSha512Digest(
                            payload, settings.apiSecret)));
    Write(request);
    nonce.Use();
  }
}
