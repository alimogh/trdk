/*******************************************************************************
 *   Created: 2017/09/23 13:42:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "FixProtocolIncomingMessages.hpp"
#include "FixProtocolMessageHandler.hpp"
#include "Common/NetworkStreamClient.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::FixProtocol;
using namespace trdk::Interaction::FixProtocol::Incoming;

namespace {
typedef NetworkStreamClient::ProtocolError ProtocolError;

template <typename It>
std::pair<size_t, It> ExtractValueAsSizeT(It source) {
  if (*source == SOH) {
    throw ProtocolError("Unsigned integer field is empty", &*source, SOH);
  }
  size_t result = 0;
  while (*source != SOH) {
    result = result * 10 + (*source++ - '0');
  }
  return std::make_pair(result, source);
}
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<Messages> Factory::Create(const Iterator &begin,
                                          const Iterator &end,
                                          const Policy &) {
  if (end - begin < 11) {
    throw ProtocolError("Message is very short", &*begin, 0);
  } else if (*(end - 1) != SOH) {
    throw ProtocolError("Message doesn't have end", &*(end - 1), SOH);
  }

  auto contentBegin = begin;

  {
    static const auto match = reinterpret_cast<const int64_t &>(*("8=FIX.4.4"));
    if (reinterpret_cast<const decltype(match) &>(*contentBegin) != match) {
      throw ProtocolError("Message has wrong protocol", &*contentBegin, 0);
    }
    contentBegin += sizeof(match);
  }
  {
    static const int32_t match = 1027146036;  // "4|9="
    if (reinterpret_cast<const decltype(match) &>(*contentBegin) != match) {
      throw ProtocolError("Message doesn't have length", &*contentBegin, 0);
    }
    contentBegin += sizeof(match);
  }

  size_t len;
  boost::tie(len, contentBegin) = ExtractValueAsSizeT(contentBegin);
  AssertEq(SOH, *contentBegin);
  Assert(contentBegin <= end);
  {
    const auto realLen = static_cast<size_t>(end - contentBegin);
    if (realLen < len + 8) {
      throw ProtocolError("Message has wrong length", &*contentBegin, 0);
    }
  }

  const auto contentEnd = contentBegin + len + 1;
  const auto messageEnd = contentEnd + 7;

  {
    const auto &checkSumTagBegin = std::prev(contentEnd);
    // |10=
    static const int32_t tagMatch = 1026568449;
    if (reinterpret_cast<const decltype(tagMatch) &>(*checkSumTagBegin) !=
        tagMatch) {
      throw ProtocolError("Message doesn't have checksum field",
                          &*checkSumTagBegin, 0);
    }
    const auto &checkSum =
        ExtractValueAsSizeT(checkSumTagBegin + sizeof(tagMatch));
    const auto &controlCheckSum = Detail::CalcCheckSum(begin, contentEnd);
    if (checkSum.first != controlCheckSum ||
        checkSum.second != std::prev(messageEnd)) {
      throw ProtocolError("Message checksum is wrong", &*contentEnd, 0);
    }
  }

  {
    // |35=
    static const int32_t match = 1026896641;
    if (reinterpret_cast<const decltype(match) &>(*contentBegin) != match) {
      throw ProtocolError("Message doesn't have type", &*contentBegin, SOH);
    }
    contentBegin += sizeof(match) + 1;
  }
  if (*contentBegin != SOH) {
    throw ProtocolError("Message doesn't have type", &*contentBegin, SOH);
  }
  const char type = *std::prev(contentBegin);

  {
    std::bitset<6> standardHeader;
    while (!standardHeader.all()) {
      size_t bit = 0;
      switch (reinterpret_cast<const int32_t &>(*contentBegin)) {
        case 1027159041:  // |49=
          bit = 1;
          break;
        case 1026962689:  // |56=
          bit = 2;
          break;
        case 1026831105:  // |34=
          bit = 3;
          break;
        case 1026700545:  // |52=
          bit = 4;
          break;
        case 1027028225:  // |57=
          bit = 5;
          break;
        case 1026569473:  // |50=
          bit = 6;
          break;
          //         default:
          //           throw ProtocolError("Message has unknown tag in Standard
          //                                   Header ",
          //                                   & *contentBegin,
          //                               SOH);
      }
      if (!bit) {
        break;
      }
      --bit;
      if (standardHeader[bit]) {
        throw ProtocolError("Standard Header has wrong format", &*contentBegin,
                            0);
      }
      contentBegin = std::find(contentBegin + sizeof(int32_t), contentEnd, SOH);
      standardHeader.set(bit);
    }
    if (contentBegin != contentEnd) {
      ++contentBegin;
    }
  }

  switch (type) {
    case MESSAGE_TYPE_LOGON:
      return boost::make_unique<Logon>(std::move(contentBegin),
                                       std::move(contentEnd),
                                       std::move(messageEnd));
    case MESSAGE_TYPE_HEARTBEAT:
      return boost::make_unique<Heartbeat>(std::move(contentBegin),
                                           std::move(contentEnd),
                                           std::move(messageEnd));
    case MESSAGE_TYPE_TEST_REQUEST:
      return boost::make_unique<TestRequest>(std::move(contentBegin),
                                             std::move(contentEnd),
                                             std::move(messageEnd));
    default:
      throw ProtocolError("Message has unknown type", &*(contentBegin - 1), 0);
  }
}

////////////////////////////////////////////////////////////////////////////////

void Logon::Handle(MessageHandler &handler) const { handler.OnLogon(*this); }

////////////////////////////////////////////////////////////////////////////////

void Heartbeat::Handle(MessageHandler &handler) const {
  handler.OnHeartbeat(*this);
}

////////////////////////////////////////////////////////////////////////////////

void TestRequest::Handle(MessageHandler &handler) const {
  handler.OnTestRequest(*this);
}

std::string TestRequest::ReadTestRequestId() const {
  // 112=
  static const int32_t tagMatch = 1026699569;
  const auto &len = GetEnd() - GetBegin();
  if (len < sizeof(tagMatch) + 1) {
    throw ProtocolError("Test Request message is too short", &*GetBegin(), 0);
  }
  const auto &end = std::prev(GetEnd());
  if (*end != SOH) {
    throw ProtocolError("Test Request message does not have end",
                        &*std::prev(GetEnd()), SOH);
  } else if (reinterpret_cast<const decltype(tagMatch) &>(*GetBegin()) !=
                 tagMatch ||
             *std::prev(GetBegin()) != SOH) {
    throw ProtocolError("Test Request message has unknown tag", &*GetBegin(),
                        '1');
  }
  return std::string((GetBegin() + sizeof(tagMatch)), end);
}

////////////////////////////////////////////////////////////////////////////////
