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
    throw ProtocolError("Massage is very short", &*begin, 0);
  } else if (*(end - 1) != SOH) {
    throw ProtocolError("Massage doesn't have end", &*(end - 1), SOH);
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

  auto contentEnd = end - 7;
  {
    // |10=
    static const int32_t match = 1026568449;
    if (reinterpret_cast<const decltype(match) &>(*(contentEnd - 1)) != match) {
      throw ProtocolError("Massage doesn't have checksum field", &*contentEnd,
                          0);
    }
  }
  {
    const auto &checkSum = ExtractValueAsSizeT(contentEnd + 3);
    Assert(checkSum.second == contentEnd + 6);
    const auto &controlCheckSum = Detail::CalcCheckSum(begin, contentEnd);
    if (checkSum.first != controlCheckSum) {
      throw ProtocolError("Massage checksum is wrong", &*contentEnd, 0);
    }
  }

  {
    size_t len;
    boost::tie(len, contentBegin) = ExtractValueAsSizeT(contentBegin);
    AssertEq(SOH, *contentBegin);
    Assert(contentBegin <= end);
    if (static_cast<size_t>(contentEnd - contentBegin) != len + 1) {
      throw ProtocolError("Massage has wrong length", &*contentBegin, 0);
    }
  }

  {
    // |35=
    static const int32_t match = 1026896641;
    if (reinterpret_cast<const decltype(match) &>(*contentBegin) != match) {
      throw ProtocolError("Message doesn't have type", &*contentBegin, SOH);
    }
    contentBegin += sizeof(match) + 2;
  }
  if (*std::prev(contentBegin) != SOH) {
    throw ProtocolError("Message doesn't have type", &*contentBegin, SOH);
  }
  const char type = *(contentBegin - 2);
  switch (type) {
    case MESSAGE_TYPE_LOGON:
      return std::make_unique<Logon>(contentBegin, contentEnd);
    default:
      throw ProtocolError("Message has unknown type", &*(contentBegin - 1), 0);
  }
}

////////////////////////////////////////////////////////////////////////////////

void Logon::Handle(MessageHandler &handler) const { handler.OnLogon(*this); }

////////////////////////////////////////////////////////////////////////////////