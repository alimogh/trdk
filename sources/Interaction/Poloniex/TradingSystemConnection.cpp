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
using namespace Poloniex;
using namespace Lib::Crypto;
namespace ptr = boost::property_tree;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

void TradingSystemConnection::Start(const AuthSettings &settings,
                                    const Events &events) {
  Handshake("/");
  Base::Start(events);
  {
    ptr::ptree request;
    request.add("command", "subscribe");
    request.add("channel", "1000");
    request.add("key", settings.apiKey);
    const auto &payload = "nonce=" + boost::lexical_cast<std::string>(
                                         (pt::microsec_clock::universal_time() -
                                          pt::ptime(gr::date(1970, 1, 1)))
                                             .total_seconds());
    request.add("payload", payload);
    request.add("sign", EncodeToHex(Hmac::CalcSha512Digest(
                            payload, settings.apiSecret)));
    Write(request);
  }
}
