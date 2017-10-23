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
#include "IncomingMessages.hpp"
#include "Policy.hpp"
#include "Security.hpp"
#include "Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::FixProtocol;
using namespace trdk::Interaction::FixProtocol::Detail;
using namespace trdk::Interaction::FixProtocol::Outgoing;

namespace fix = trdk::Interaction::FixProtocol;
namespace out = trdk::Interaction::FixProtocol::Outgoing;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

////////////////////////////////////////////////////////////////////////////////

namespace {

std::string ConvertToTagValue(const pt::ptime &source) {
  // 20170117 - 08:03:04:
  return gr::to_iso_string(source.date()) + "-" +
         pt::to_simple_string(source.time_of_day());
}
}

////////////////////////////////////////////////////////////////////////////////

MessageSequenceNumber StandardHeader::TakeMessageSequenceNumber() {
  return m_nextMessageSequenceNumber++;
}

std::vector<char> StandardHeader::Export(
    const MessageType &messageType,
    const std::string &messageSequenceNumber,
    size_t messageLen,
    unsigned char soh) const {
  std::vector<char> result;
  {
    const std::string sub("8=FIX.4.4");
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }

  const auto &settings = GetSettings();
  {
    // 46 bytes:
    // without custom fields:
    //   35=A|49=|56=|34=|52=20170117-08:03:04|57=|50=|
    // full
    // message:8=FIX.4.4|9=126|35=A|49=theBroker.12345|56=CSERVER|34=1|52=20170117-08:03:04|57=TRADE|50=any_string|
    messageLen += 46;
    messageLen += messageSequenceNumber.size();  // 34
    messageLen += settings.senderCompId.size();  // 49
    messageLen += settings.targetCompId.size();  // 56
    messageLen += settings.targetSubId.size();   // 57
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
    const std::string sub("34=" + messageSequenceNumber);
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  {
    // "20170117 - 08:03:04":
    const std::string sub(
        "52=" + ConvertToTagValue(GetSettings().policy->GetCurrentTime()));
    AssertEq(20, sub.size());
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  {
    const std::string sub("57=" + settings.targetSubId);
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  {
    const std::string sub("50=" + settings.senderSubId);
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }

  return result;
}

////////////////////////////////////////////////////////////////////////////////

out::Message::Message(StandardHeader &standardHeader)
    : m_standardHeader(standardHeader),
      m_sequenceNumber(m_standardHeader.TakeMessageSequenceNumber()),
      m_sequenceNumberCode(boost::lexical_cast<std::string>(m_sequenceNumber)) {
}

std::vector<char> out::Message::Export(const MessageType &messageType,
                                       size_t messageLen,
                                       unsigned char soh) const {
  return GetStandardHeader().Export(messageType, GetSequenceNumberCode(),
                                    messageLen, soh);
}

////////////////////////////////////////////////////////////////////////////////

std::vector<char> Logon::Export(unsigned char soh) const {
  const auto &settings = GetStandardHeader().GetSettings();

  // 28 bytes:
  // without custom fields:
  //   98=0|108=30|141=Y|553=|554=|
  // full message:
  //   98=0|108=30|141=Y|553=12345|554=passw0rd!|
  auto result =
      Export(MESSAGE_TYPE_LOGON,
             28 + settings.username.size() + settings.password.size(), soh);

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

////////////////////////////////////////////////////////////////////////////////

Heartbeat::Heartbeat(const Incoming::TestRequest &testRequest,
                     StandardHeader &standardHeader)
    : Base(standardHeader), m_testRequestId(testRequest.ReadTestReqId()) {}

std::vector<char> Heartbeat::Export(unsigned char soh) const {
  auto result =
      Export(MESSAGE_TYPE_HEARTBEAT,
             m_testRequestId.empty() ? 0 : m_testRequestId.size() + 5, soh);
  if (!m_testRequestId.empty()) {
    const std::string sub("112=" + m_testRequestId);
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  WriteStandardTrailer(result, soh);
  return result;
}

////////////////////////////////////////////////////////////////////////////////

namespace {
std::string ResolveSecurityFixId(const trdk::Security &security) {
  const auto *const fixSecurity =
      dynamic_cast<const fix::Security *>(&security);
  if (!fixSecurity) {
    throw MethodIsNotImplementedException(
        "Work with non FIX protocol security is not implemented yet in the "
        "module FIX protocol");
  }
  return fixSecurity->GetFixIdCode();
}
}

SecurityMessage::SecurityMessage(const trdk::Security &security,
                                 StandardHeader &standardHeader)
    : Base(standardHeader),
      m_security(security),
      m_symbolId(ResolveSecurityFixId(m_security)) {}

////////////////////////////////////////////////////////////////////////////////

namespace {
boost::atomic_uintmax_t nextMarketDataRequestId(1);
}
MarketDataMessage::MarketDataMessage(const fix::Security &security,
                                     StandardHeader &header)
    : Base(security, header),
      m_marketDataRequestId(
          boost::lexical_cast<std::string>(nextMarketDataRequestId++)) {}

std::vector<char> MarketDataRequest::Export(unsigned char soh) const {
  const auto &marketDataRequestId = GetMarketDataRequestId();

  // 51 bytes:
  // without custom fields:
  //  262=|263=1|264=1|265=1|146=1|55=|267=2|269=0|269=1|
  // full message:
  //  262=876316403|263=1|264=1|265=1|146=1|55=1|267=2|269=0|269=1|

  auto result =
      Export(MESSAGE_TYPE_MARKET_DATA_REQUEST,
             51 + marketDataRequestId.size() + GetSymbolId().size(), soh);

  // MDReqID:
  {
    const std::string sub("262=" + marketDataRequestId);
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  // SubscriptionRequestType:
  {
    // 1 = Snapshot plus updates (subscribe), 2 = Disable previous snapshot
    // plus update request(unsubscribe).
    const std::string sub("263=1");
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  // MarketDepth
  {
    // Full book will be provided, 0 = Depth subscription; 1 = Spot
    // subscription.
    const std::string sub("264=1");
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  // MDUpdateTy
  {
    // Only Incremental refresh is supported (1).
    const std::string sub("265=1");
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  // NoRelatedSym
  {
    const std::string sub("146=1");
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  // Symbol
  {
    const std::string sub("55=" + GetSymbolId());
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  // NoMDEntryTypes
  {
    // Always set to 2 (both bid and ask will be sent)
    const std::string sub("267=2");
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  // NoMDEntryTypes
  {
    // Bid
    const std::string sub("269=0");
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  {
    // Offer
    const std::string sub("269=1");
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }

  WriteStandardTrailer(result, soh);

  return result;
}

////////////////////////////////////////////////////////////////////////////////

NewOrderSingle::NewOrderSingle(const trdk::Security &security,
                               const OrderSide &side,
                               const Qty &qty,
                               const Price &price,
                               StandardHeader &standardHeader)
    : Base(security, standardHeader),
      m_side(side == ORDER_SIDE_BUY ? '1' : '2'),
      m_qty(boost::lexical_cast<std::string>(qty)),
      m_price(boost::lexical_cast<std::string>(price)),
      m_transactTime(ConvertToTagValue(
          GetStandardHeader().GetSettings().policy->GetCurrentTime())),
      m_customContentSize(GetSequenceNumberCode().size() +
                          GetSymbolId().size() + 1 + m_qty.size() +
                          m_price.size() + m_transactTime.size()) {}

std::vector<char> NewOrderSingle::Export(unsigned char soh) const {
  // 25 bytes:
  // without custom fields:
  //  11=|55=|54=|60=|40=2|38=|44=|
  // full message:
  //  11=876316397|55=1|54=1|60=20170117-10:02:14|40=1|38=10000|55=123|

  auto result =
      Export(MESSAGE_TYPE_NEW_ORDER_SINGLE, 29 + m_customContentSize, soh);
  // ClOrdID:
  {
    const std::string sub("11=" + GetSequenceNumberCode());
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  // Symbol
  {
    const std::string sub("55=" + GetSymbolId());
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  // Side:
  {
    const std::string sub("54=");
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(m_side);
    result.emplace_back(soh);
  }
  // TransactTime:
  {
    const std::string sub("60=" + m_transactTime);
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  // OrdType:
  {
    const std::string sub("40=2");
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  // OrderQty:
  {
    const std::string sub("38=" + m_qty);
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }
  // Price:
  {
    const std::string sub("44=" + m_price);
    std::copy(sub.cbegin(), sub.cend(), std::back_inserter(result));
    result.emplace_back(soh);
  }

  WriteStandardTrailer(result, soh);

  return result;
}

////////////////////////////////////////////////////////////////////////////////