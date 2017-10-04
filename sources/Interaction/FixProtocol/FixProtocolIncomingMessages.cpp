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
#include "FixProtocolIncomingMessagesFabric.hpp"
#include "FixProtocolMarketDataSource.hpp"
#include "FixProtocolPolicy.hpp"
#include "Common/NetworkStreamClient.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Interaction::FixProtocol;
using namespace trdk::Interaction::FixProtocol::Detail;
using namespace trdk::Interaction::FixProtocol::Incoming;
using namespace trdk::Interaction::FixProtocol::Incoming::Detail;

namespace pt = boost::posix_time;
namespace gr = boost::gregorian;
namespace fix = trdk::Interaction::FixProtocol;

namespace {
typedef NetworkStreamClient::ProtocolError ProtocolError;

template <typename Result, typename It>
Result ReadIntValue(It &source) {
  if (*source == SOH) {
    throw ProtocolError("Integer field is empty", &*source, SOH);
  }
  Result result = 0;
  do {
    result = result * 10 + (*source++ - '0');
  } while (*source != SOH);
  return result;
}
template <typename Result, typename It>
Result ReadIntValue(It &source, char delimiter) {
  if (*source == SOH || *source == delimiter) {
    throw ProtocolError("Unsigned integer field is empty", &*source, delimiter);
  }
  Result result = 0;
  do {
    result = result * 10 + (*source++ - '0');
  } while (*source != SOH && *source != delimiter);
  return result;
}
template <typename Result, typename It>
Result ReadDoubleValue(It &source) {
  if (*source == SOH) {
    throw ProtocolError("Double field is empty", &*source, SOH);
  }
  Result result = .0;

  // https://tinodidriksen.com/2011/05/cpp-convert-string-to-double-speed/
  // https://tinodidriksen.com/uploads/code/cpp/speed-string-to-double.cpp
  // (native)

  bool isNegative = false;
  if (*source == '-') {
    isNegative = true;
    ++source;
  }

  while (*source >= '0' && *source <= '9') {
    result = (result * 10.0) + (*source - '0');
    ++source;
  }
  if (*source == '.') {
    double f = 0.0;
    int n = 0;
    ++source;
    while (*source >= '0' && *source <= '9') {
      f = (f * 10.0) + (*source - '0');
      ++source;
      ++n;
    }
    if (*source != SOH) {
      throw ProtocolError("Double has wrong format", &*source, SOH);
    }
    result += f / std::pow(10.0, n);
  }

  if (isNegative) {
    result = -result;
  }

  return result;
}

template <typename Result, typename It, typename TagMatch>
Result ReadIntTag(const TagMatch &tagMatch, It &source, const It &messageEnd) {
  Assert(source <= messageEnd);
  const auto &len = messageEnd - source;
  if (len < sizeof(tagMatch) + 1 + 1) {  // "1 digit"  + SOH
    throw ProtocolError("Integer value tag buffer is too short", &*source, 0);
  }
  if (reinterpret_cast<const decltype(tagMatch) &>(*source) != tagMatch) {
    throw ProtocolError("Unknown integer value tag", &*source, 0);
  }
  source += sizeof(tagMatch);
  const auto &result = ReadIntValue<Result>(source);
  ++source;
  return result;
}

template <typename Result, typename It, typename TagMatch>
Result ReadDoubleTag(const TagMatch &tagMatch,
                     It &source,
                     const It &messageEnd) {
  Assert(source <= messageEnd);
  const auto &len = messageEnd - source;
  if (len < sizeof(tagMatch) + 1 + 1) {  // "1 digit" + SOH
    throw ProtocolError("Double value tag buffer is too short", &*source, 0);
  }
  if (reinterpret_cast<const decltype(tagMatch) &>(*source) != tagMatch) {
    throw ProtocolError("Unknown double value tag", &*source, 0);
  }
  source += sizeof(tagMatch);
  const auto &result = ReadDoubleValue<Result>(source);
  ++source;
  return result;
}
template <typename It, typename TagMatch>
Price ReadPriceTag(const TagMatch &tagMatch, It &source, const It &messageEnd) {
  return ReadDoubleTag<double>(tagMatch, source, messageEnd);
}

template <typename It, typename TagMatch>
char ReadCharTag(const TagMatch &tagMatch, It &source, const It &messageEnd) {
  Assert(source <= messageEnd);
  const auto &len = messageEnd - source;
  if (len < sizeof(tagMatch) + 1 + 1) {  // char  + SOH
    throw ProtocolError("Char value tag buffer has wrong length", &*source, 0);
  }
  if (reinterpret_cast<const decltype(tagMatch) &>(*source) != tagMatch) {
    throw ProtocolError("Unknown char value tag", &*source, 0);
  }
  source += sizeof(tagMatch) + 2;
  if (*std::prev(source) != SOH) {
    throw ProtocolError("Char value tag buffer has wrong length",
                        &*std::prev(source), SOH);
  }
  return *(source - 2);
}

template <typename It, typename TagMatch>
std::string ReadStringTagFromSoh(const TagMatch &tagMatch,
                                 It &source,
                                 const It &messageEnd) {
  Assert(source <= messageEnd);
  const auto &len = messageEnd - source;
  if (len < sizeof(tagMatch) + 0 + 1) {  // "empty content"  + SOH
    throw ProtocolError("String value tag buffer is too short", &*source, 0);
  }

  if (reinterpret_cast<const decltype(tagMatch) &>(*source) != tagMatch) {
    throw ProtocolError("Unknown string value tag", &*source, 0);
  }

  std::string result;
  for (source += sizeof(tagMatch);; ++source) {
    if (source == messageEnd) {
      throw ProtocolError("String value buffer doesn't have end",
                          &*std::prev(source), SOH);
    }
    const auto &ch = *source;
    if (ch == SOH) {
      ++source;
      break;
    }
    result.push_back(std::move(ch));
  }

  return result;
}

template <typename It, typename TagMatch>
std::string ReadStringTag(const TagMatch &tagMatch,
                          It &source,
                          const It &messageEnd) {
  if (*std::prev(source) != SOH) {
    throw ProtocolError("String value buffer doesn't have begin",
                        &*std::prev(source), SOH);
  }
  return ReadStringTagFromSoh(tagMatch, source, messageEnd);
}

template <typename Result, typename It, typename TagMatch>
Result FindAndReadIntTag(const TagMatch &tagMatch, It &source, const It &end) {
  const auto begin = source;
  for (; source + sizeof(tagMatch) < end;) {
    if (reinterpret_cast<const decltype(tagMatch) &>(*source) == tagMatch) {
      source += sizeof(tagMatch);
      const auto &result = ReadIntValue<Result>(source);
      ++source;
      return result;
    }
    source = std::find(source + sizeof(tagMatch), end, SOH);
    if (source != end) {
      ++source;
    }
  }
  throw ProtocolError("Message doesn't have required tag with integer value",
                      &*begin, 0);
}
template <typename Result, typename It, typename TagMatch>
Result FindAndReadIntTagFromSoh(const TagMatch &tagMatch,
                                It &source,
                                const It &end) {
  const auto begin = source;
  for (; source + sizeof(tagMatch) < end;
       source = std::find(source + sizeof(tagMatch), end, SOH)) {
    if (reinterpret_cast<const decltype(tagMatch) &>(*source) == tagMatch) {
      source += sizeof(tagMatch);
      const auto &result = ReadIntValue<Result>(source);
      Assert(source < end);
      AssertEq(SOH, *source);
      ++source;
      return result;
    }
  }
  throw ProtocolError("Message doesn't have required tag with integer value",
                      &*begin, 0);
}

template <typename Result, typename It, typename TagMatch>
std::pair<Result, It> FindAndReadCharTag(const TagMatch &tagMatch,
                                         const It &begin,
                                         const It &end) {
  for (auto it = begin; it + sizeof(tagMatch) < end;) {
    if (reinterpret_cast<const decltype(tagMatch) &>(*it) == tagMatch) {
      return ReadIntValue<Result>(it + sizeof(tagMatch));
    }
    it = std::find(it + sizeof(tagMatch), end, SOH);
    if (it != end) {
      ++it;
    }
  }
  throw ProtocolError("Message doesn't have required tag with char value",
                      &*begin, 0);
}

template <typename It, typename TagMatch>
std::string FindAndReadStringTagFromSoh(const TagMatch &tagMatch,
                                        It &source,
                                        const It &end) {
  const auto begin = source;
  for (; source + sizeof(tagMatch) < end;
       source = std::find(source + sizeof(tagMatch), end, SOH)) {
    if (reinterpret_cast<const decltype(tagMatch) &>(*source) == tagMatch) {
      source += sizeof(tagMatch);
      std::string result;
      for (;; ++source) {
        if (source == end) {
          throw ProtocolError("String value buffer doesn't have end",
                              &*std::prev(source), SOH);
        }
        const auto &ch = *source;
        if (ch == SOH) {
          ++source;
          break;
        }
        result.push_back(std::move(ch));
      }
      return result;
    }
  }
  throw ProtocolError("Message doesn't have required tag with string value",
                      &*begin, 0);
}
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<Incoming::Message> Factory::Create(const Iterator &begin,
                                                   const Iterator &end,
                                                   const Policy &policy) {
  if (end - begin < 11) {
    throw ProtocolError("Message is very short", &*begin, 0);
  } else if (*(end - 1) != SOH) {
    throw ProtocolError("Message doesn't have end", &*(end - 1), SOH);
  }

  MessagesParams params{begin};

  {
    static const auto match = reinterpret_cast<const int64_t &>(*("8=FIX.4.4"));
    if (reinterpret_cast<const decltype(match) &>(*params.begin) != match) {
      throw ProtocolError("Message has wrong protocol", &*params.begin, 0);
    }
    params.begin += sizeof(match);
  }
  {
    static const int32_t match = 1027146036;  // "4|9="
    if (reinterpret_cast<const decltype(match) &>(*params.begin) != match) {
      throw ProtocolError("Message doesn't have length", &*params.begin, 0);
    }
    params.begin += sizeof(match);
  }

  const auto &len = ReadIntValue<size_t>(params.begin);
  AssertEq(SOH, *params.begin);
  Assert(params.begin <= end);
  {
    const auto realLen = static_cast<size_t>(end - params.begin);
    if (realLen < len + 8) {
      throw ProtocolError("Message has wrong length", &*params.begin, 0);
    }
  }

  params.end = params.begin + len + 1;
  params.messageEnd = params.end + 7;

  {
    auto checkSumIt = std::prev(params.end);
    // |10=
    static const int32_t tagMatch = 1026568449;
    if (reinterpret_cast<const decltype(tagMatch) &>(*checkSumIt) != tagMatch) {
      throw ProtocolError("Message doesn't have checksum field", &*checkSumIt,
                          0);
    }
    checkSumIt += sizeof(tagMatch);
    const auto &checkSum = ReadIntValue<size_t>(checkSumIt);
    const auto &controlCheckSum = CalcCheckSum(begin, params.end);
    if (checkSum != controlCheckSum ||
        checkSumIt != std::prev(params.messageEnd)) {
      throw ProtocolError("Message checksum is wrong", &*params.end, 0);
    }
  }

  {
    // |35=
    static const int32_t match = 1026896641;
    if (reinterpret_cast<const decltype(match) &>(*params.begin) != match) {
      throw ProtocolError("Message doesn't have type", &*params.begin, SOH);
    }
    params.begin += sizeof(match) + 1;
  }
  if (*params.begin != SOH) {
    throw ProtocolError("Message doesn't have type", &*params.begin, SOH);
  }
  const char type = *std::prev(params.begin);

  {
    std::bitset<6> standardHeader;
    while (!standardHeader.all()) {
      size_t bit = 0;
      typedef int32_t TagMatch;
      switch (reinterpret_cast<const TagMatch &>(*params.begin)) {
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
          AssertEq(params.time, pt::not_a_date_time);
          try {
            auto it = params.begin + sizeof(TagMatch);
            const auto &date = ReadIntValue<long>(it, '-');
            ++it;
            const auto &hours = ReadIntValue<long>(it, ':');
            ++it;
            const auto &minutes = ReadIntValue<long>(it, ':');
            ++it;
            const auto &seconds = ReadIntValue<long>(it, '.');
            ++it;
            const auto fieldStart = it;
            const auto &mili = ReadIntValue<long>(it);
            if (std::distance(fieldStart, it) != 3) {
              throw ProtocolError("Wrong format of milliseconds", &*fieldStart,
                                  0);
            }
            params.time = policy.ConvertFromRemoteTime(pt::ptime(
                ConvertToDate(date),
                pt::time_duration(hours, minutes, seconds, mili * 1000)));
          } catch (const ProtocolError &ex) {
            boost::format error("Failed to parse message time: \"%1%\"");
            error % ex.what();
            throw ProtocolError(error.str().c_str(), ex.GetBufferAddress(),
                                ex.GetExpectedByte());
          } catch (const std::exception &) {
            throw ProtocolError("Failed to parse message time",
                                &*(params.begin + sizeof(TagMatch)), 0);
          }
          bit = 4;
          break;
        case 1027028225:  // |57=
          bit = 5;
          break;
        case 1026569473:  // |50=
          bit = 6;
          break;
        default:
          throw ProtocolError("Message has unknown tag in Standard Header",
                              &*params.begin, SOH);
      }
      if (!bit) {
        break;
      }
      --bit;
      if (standardHeader[bit]) {
        throw ProtocolError("Standard Header has wrong format", &*params.begin,
                            0);
      }
      params.begin = std::find(params.begin + sizeof(int32_t), params.end, SOH);
      standardHeader.set(bit);
    }
    if (params.begin != params.end) {
      ++params.begin;
    }
  }

  switch (type) {
    case MESSAGE_TYPE_LOGON:
      return boost::make_unique<Logon>(std::move(params));
    case MESSAGE_TYPE_LOGOUT:
      return boost::make_unique<Logout>(std::move(params));
    case MESSAGE_TYPE_HEARTBEAT:
      return boost::make_unique<Heartbeat>(std::move(params));
    case MESSAGE_TYPE_TEST_REQUEST:
      return boost::make_unique<TestRequest>(std::move(params));
    case MESSAGE_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH:
      return boost::make_unique<MarketDataSnapshotFullRefresh>(
          std::move(params));
    case MESSAGE_TYPE_MARKET_DATA_INCREMENTAL_REFRESH:
      return boost::make_unique<MarketDataIncrementalRefresh>(
          std::move(params));
    case MESSAGE_TYPE_RESEND_REQUEST:
      return boost::make_unique<ResendRequest>(std::move(params));
    case MESSAGE_TYPE_REJECT:
      return boost::make_unique<Reject>(std::move(params));
    case MESSAGE_TYPE_BUSINESS_MESSAGE_REJECT:
      return boost::make_unique<BusinessMessageReject>(std::move(params));
    default: {
      boost::format message("Message has unknown type '%1%'");
      message % type;
      throw ProtocolError(message.str().c_str(), &*(params.begin - 1), 0);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

Incoming::Message::Message(const Detail::MessagesParams &&params)
    : m_params(std::move(params)), m_unreadBegin(m_params.begin) {
  Assert(m_params.end <= m_params.messageEnd);
  Assert(m_unreadBegin <= m_params.end);
  AssertNe(SOH, *m_unreadBegin);
  AssertEq(SOH, *std::prev(m_unreadBegin));
  AssertEq(SOH, *std::prev(m_params.messageEnd));
  AssertNe(boost::posix_time::not_a_date_time, GetTime());
}

void Incoming::Message::ResetReadingState() const {
  m_unreadBegin = m_params.begin;
}

void Incoming::Message::SetUnreadBegin(const Iterator &value) const {
  Assert(m_unreadBegin < value);
  AssertEq(SOH, *std::prev(value));
  m_unreadBegin = value;
}

std::string Incoming::Message::FindAndReadStringFromSoh(
    int32_t tagMatch) const {
  try {
    auto it = std::prev(GetUnreadBegin());
    const auto &result = FindAndReadStringTagFromSoh(tagMatch, it, GetEnd());
    SetUnreadBegin(it);
    return result;
  } catch (const ProtocolError &ex) {
    // Tag name may be extracted from the tagMatch.
    boost::format error(
        "Failed to read a string value from message field: \"%1%\"");
    error % ex.what();
    throw ProtocolError(error.str().c_str(), ex.GetBufferAddress(),
                        ex.GetExpectedByte());
  } catch (const std::exception &ex) {
    // Tag name may be extracted from the tagMatch.
    boost::format error(
        "Failed to read a string value from message field: \"%1%\"");
    error % ex.what();
    throw ProtocolError(error.str().c_str(), &*GetUnreadBegin(), 0);
  }
}

template <typename Result>
Result Incoming::Message::FindAndReadInt(int32_t tagMatch) const {
  try {
    return FindAndReadIntTag<Result>(tagMatch, GetUnreadBeginRef(), GetEnd());
  } catch (const ProtocolError &ex) {
    // Tag name may be extracted from the tagMatch.
    boost::format error(
        "Failed to read an integer value from message field: \"%1%\"");
    error % ex.what();
    throw ProtocolError(error.str().c_str(), ex.GetBufferAddress(),
                        ex.GetExpectedByte());
  } catch (const std::exception &ex) {
    // Tag name may be extracted from the tagMatch.
    boost::format error(
        "Failed to read an integer value string from message field: \"%1%\"");
    error % ex.what();
    throw ProtocolError(error.str().c_str(), &*GetUnreadBegin(), 0);
  }
}

template <typename Result>
Result Incoming::Message::FindAndReadIntFromSoh(int32_t tagMatch) const {
  try {
    auto it = std::prev(GetUnreadBegin());
    const auto &result =
        FindAndReadIntTagFromSoh<Result>(tagMatch, it, GetEnd());
    SetUnreadBegin(it);
    return result;
  } catch (const ProtocolError &ex) {
    // Tag name may be extracted from the tagMatch.
    boost::format error(
        "Failed to read an integer value from message field: \"%1%\"");
    error % ex.what();
    throw ProtocolError(error.str().c_str(), ex.GetBufferAddress(),
                        ex.GetExpectedByte());
  } catch (const std::exception &ex) {
    // Tag name may be extracted from the tagMatch.
    boost::format error(
        "Failed to read an integer value string from message field: \"%1%\"");
    error % ex.what();
    throw ProtocolError(error.str().c_str(), &*GetUnreadBegin(), 0);
  }
}

MessageSequenceNumber Incoming::Message::ReadRefSeqNum() const {
  // |45=
  return FindAndReadIntFromSoh<MessageSequenceNumber>(
      static_cast<int32_t>(1026896897));
}

MessageSequenceNumber Incoming::Message::ReadBusinessRejectRefId() const {
  // 379=
  return FindAndReadInt<MessageSequenceNumber>(
      static_cast<int32_t>(1027159859));
}

std::string Incoming::Message::ReadText() const {
  // |58=
  return FindAndReadStringFromSoh(static_cast<int32_t>(1027093761));
}

////////////////////////////////////////////////////////////////////////////////

void Logon::Handle(Handler &handler,
                   NetworkStreamClient &client,
                   const Milestones &) {
  handler.OnLogon(*this, client);
}

////////////////////////////////////////////////////////////////////////////////

void Logout::Handle(Handler &handler,
                    NetworkStreamClient &client,
                    const Milestones &) {
  handler.OnLogout(*this, client);
}

////////////////////////////////////////////////////////////////////////////////

void Heartbeat::Handle(Handler &handler,
                       NetworkStreamClient &client,
                       const Milestones &) {
  handler.OnHeartbeat(*this, client);
}

////////////////////////////////////////////////////////////////////////////////

void TestRequest::Handle(Handler &handler,
                         NetworkStreamClient &client,
                         const Milestones &) {
  handler.OnTestRequest(*this, client);
}

std::string TestRequest::ReadTestReqId() const {
  // 112=
  static const int32_t tagMatch = 1026699569;
  try {
    return ReadStringTag(tagMatch, GetUnreadBeginRef(), GetEnd());
  } catch (const ProtocolError &ex) {
    boost::format error(
        "Failed to read Test Request ID from Test Request message: \"%1%\"");
    error % ex.what();
    throw ProtocolError(error.str().c_str(), ex.GetBufferAddress(),
                        ex.GetExpectedByte());
  }
}

////////////////////////////////////////////////////////////////////////////////

void ResendRequest::Handle(Handler &handler,
                           NetworkStreamClient &client,
                           const Milestones &) {
  handler.OnResendRequest(*this, client);
}

////////////////////////////////////////////////////////////////////////////////

void Reject::Handle(Handler &handler,
                    NetworkStreamClient &client,
                    const Milestones &) {
  handler.OnReject(*this, client);
}

////////////////////////////////////////////////////////////////////////////////

fix::Security &SecurityMessage::ReadSymbol(
    fix::MarketDataSource &source) const {
  try {
    // |55=
    return source.GetSecurityByFixId(
        FindAndReadIntFromSoh<size_t>(static_cast<int32_t>(1026897153)));
  } catch (const ProtocolError &ex) {
    boost::format error("Failed to read message symbol: \"%1%\"");
    error % ex.what();
    throw ProtocolError(error.str().c_str(), ex.GetBufferAddress(),
                        ex.GetExpectedByte());
  } catch (const std::exception &ex) {
    boost::format error("Failed to read message symbol: \"%1%\"");
    error % ex.what();
    throw ProtocolError(error.str().c_str(), &*GetUnreadBegin(), 0);
  }
}

////////////////////////////////////////////////////////////////////////////////

void MarketDataSnapshotFullRefresh::ReadEachMdEntry(
    const boost::function<void(Level1TickValue &&, bool isLast)> &callback)
    const {
  try {
    auto numberOfEntries =
        FindAndReadIntTag<size_t>(reinterpret_cast<const int32_t &>("268="),
                                  GetUnreadBeginRef(), GetEnd());
    if (numberOfEntries <= 0) {
      throw ProtocolError("Market Data Entry list is empty", &*GetUnreadBegin(),
                          0);
    }

    do {
      Level1TickType tickType;
      {
        static const auto entryTypeTag =
            reinterpret_cast<const int32_t &>("269=");
        switch (ReadCharTag(entryTypeTag, GetUnreadBeginRef(), GetEnd())) {
          case '0':
            tickType = LEVEL1_TICK_BID_PRICE;
            break;
          case '1':
            tickType = LEVEL1_TICK_ASK_PRICE;
            break;
          default:
            throw ProtocolError("Unknown Market Data Entry type",
                                &*(GetUnreadBeginRef() - 2), '0');
        }
      }

      const auto &price =
          ReadPriceTag(reinterpret_cast<const int32_t &>("270="),
                       GetUnreadBeginRef(), GetEnd());

      callback(Level1TickValue::Create(std::move(tickType), std::move(price)),
               numberOfEntries == 1);

    } while (--numberOfEntries);

  } catch (const ProtocolError &ex) {
    boost::format error("Failed to read Market Data Entries: \"%1%\"");
    error % ex.what();
    throw ProtocolError(error.str().c_str(), ex.GetBufferAddress(),
                        ex.GetExpectedByte());
  } catch (const std::exception &ex) {
    boost::format error("Failed to read Market Data Entries: \"%1%\"");
    error % ex.what();
    throw ProtocolError(error.str().c_str(), &*GetUnreadBegin(), 0);
  }
}

void MarketDataSnapshotFullRefresh::Handle(Handler &handler,
                                           NetworkStreamClient &client,
                                           const Milestones &delayMeasurement) {
  handler.OnMarketDataSnapshotFullRefresh(*this, client, delayMeasurement);
}

void MarketDataIncrementalRefresh::Handle(Handler &handler,
                                          NetworkStreamClient &client,
                                          const Milestones &delayMeasurement) {
  handler.OnMarketDataIncrementalRefresh(*this, client, delayMeasurement);
}

////////////////////////////////////////////////////////////////////////////////

void BusinessMessageReject::Handle(Handler &handler,
                                   NetworkStreamClient &client,
                                   const Milestones &delayMeasurement) {
  handler.OnBusinessMessageReject(*this, client, delayMeasurement);
}

////////////////////////////////////////////////////////////////////////////////
