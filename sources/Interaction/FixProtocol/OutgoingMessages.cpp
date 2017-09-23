/*******************************************************************************
 *   Created: 2017/09/21 23:47:25
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "OutgoingMessages.hpp"
#include "MarketDataSource.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::FixProtocol;
using namespace trdk::Interaction::FixProtocol::Outgoing;

namespace out = trdk::Interaction::FixProtocol::Outgoing;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

////////////////////////////////////////////////////////////////////////////////

std::vector<char> StandardHeader::Export(const MessageType &messageType,
                                         size_t messageLen,
                                         unsigned char soh) const {
  std::vector<char> result;
  {
    const std::string sub("8=FIX.4.4");
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }

  // Additional session qualifier. Possible values are : "QUOTE", "TRADE".
  const std::string targetSubId = "QUOTE";
  const auto &settings = GetSource().GetSettings();
  {
    // 47 bytes:
    // without custom fields:
    //   35=A|49=|56=|34=1|52=20170117-08:03:04|57=|50=|
    // full
    // message:8=FIX.4.4|9=126|35=A|49=theBroker.12345|56=CSERVER|34=1|52=20170117-08:03:04|57=TRADE|50=any_string|
    messageLen += 47;
    messageLen += settings.senderCompId.size();  // 49
    messageLen += settings.targetCompId.size();  // 56
    messageLen += targetSubId.size();            // 57
    messageLen += settings.senderSubId.size();   // 50
    result.reserve(messageLen + 20);
    const std::string sub("9=" + boost::lexical_cast<std::string>(messageLen));
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }

  {
    std::string sub("35=");
    sub.push_back(static_cast<char>(messageType));
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  {
    const std::string sub("49=" + settings.senderCompId);
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  {
    const std::string sub("56=" + settings.targetCompId);
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  {
    const std::string sub("34=1");
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  {
    const auto &now = pt::second_clock::universal_time();
    // "20170117 - 08:03:04":
    const std::string sub("52=" + gr::to_iso_string(now.date()) + "-" +
                          pt::to_simple_string(now.time_of_day()));
    AssertEq(20, sub.size());
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  {
    const std::string sub("57=" + targetSubId);
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  {
    const std::string sub("57=" + settings.senderSubId);
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }

  return result;
}

////////////////////////////////////////////////////////////////////////////////

std::vector<char> out::Message::Export(const MessageType &messageType,
                                       size_t messageLen,
                                       unsigned char soh) const {
  return GetStandardHeader().Export(messageType, messageLen, soh);
}

////////////////////////////////////////////////////////////////////////////////

std::vector<char> Logon::Export(unsigned char soh) const {
  const auto &settings = GetStandardHeader().GetSource().GetSettings();

  // 28 bytes:
  // without custom fields:
  //   98=0|108=30|141=Y|553=|554=|
  // full message:
  //   98=0|108=30|141=Y|553=12345|554=passw0rd!|
  const size_t messageLen =
      28 + settings.username.size() + settings.password.size();

  auto result = Export(MESSAGE_TYPE_LOGON, messageLen, soh);

  {
    const std::string sub("98=0");
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  {
    const std::string sub("108=30");
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  {
    const std::string sub("141=Y");
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  {
    const std::string sub("553=" + settings.username);
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  {
    const std::string sub("554=" + settings.password);
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }

  WriteStandardTrailer(result, soh);

  return result;
}
