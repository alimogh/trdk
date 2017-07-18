/**************************************************************************
 *   Created: 2016/08/29 17:12:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "HttpStreamClient.hpp"
#include "NetworkStreamClientService.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;

namespace {

enum State { STATE_NO_REQUEST, STATE_REQUEST_SENT, STATE_HEADER_RECEIVED };
}

class HttpStreamClient::Implementation : private boost::noncopyable {
 public:
  HttpStreamClient &m_self;
  const std::string m_host;
  State m_state;

  explicit Implementation(HttpStreamClient &self, const std::string &host)
      : m_self(self), m_host(host), m_state(STATE_NO_REQUEST) {}

  void HandleHeaders(
      const boost::split_iterator<Buffer::const_iterator> &range) {
    if (!boost::starts_with(*range, "HTTP/1.1 200 OK\r\n")) {
      boost::format message(
          "%1%Request response was not returned with success: \"%2%\".");
      message % m_self.GetService().GetLogTag() %
          std::string(
              boost::begin(*range),
              std::find(boost::begin(*range), boost::end(*range), '\r'));
      m_self.LogError(message.str());
      throw Exception("Request response was not returned with success");
    }
  }
};

HttpStreamClient::HttpStreamClient(NetworkStreamClientService &service,
                                   const std::string &host,
                                   size_t port)
    : NetworkStreamClient(service, host, port),
      m_pimpl(new Implementation(*this, host)) {}

HttpStreamClient::~HttpStreamClient() {}

void HttpStreamClient::Request(
    const std::string &request,
    const boost::function<void(void)> &onRequestCallback) {
  const BufferLock lock(LockDataExchange());
  if (m_pimpl->m_state != STATE_NO_REQUEST) {
    throw Exception(
        "Failed to start new request as previous request is not completed");
  }
  std::ostringstream headers;
  headers << "GET " << request << " HTTP/1.1\r\n"
          << "Host: " << m_pimpl->m_host << "\r\n"
          << "Accept: text/html\r\n"
          << "\r\n";
  Send(headers.str());
  onRequestCallback();
  m_pimpl->m_state = STATE_REQUEST_SENT;
}

void HttpStreamClient::OnStart() {}

HttpStreamClient::Buffer::const_iterator
HttpStreamClient::FindLastMessageLastByte(
    const Buffer::const_iterator &begin,
    const Buffer::const_iterator &,
    const Buffer::const_iterator &end) const {
  Assert(begin != end);
  AssertLt(STATE_NO_REQUEST, m_pimpl->m_state);

  auto range = boost::make_iterator_range(begin, end);
  auto result = end;

  if (m_pimpl->m_state == STATE_REQUEST_SENT) {
    const auto &header = boost::make_find_iterator(
        range, boost::first_finder("\r\n\r\n", boost::is_equal()));
    if (header == boost::find_iterator<Buffer::const_iterator>()) {
      return result;
    }
    result = std::prev(boost::end(*header));
    range = boost::make_iterator_range(boost::end(*header), end);
  }

  for (auto chunk = boost::make_find_iterator(
           range, boost::first_finder("\r\n", boost::is_equal()));
       !chunk.eof();) {
    if (chunk->empty()) {
      throw ProtocolError("Failed to read chunk size", &*boost::begin(range),
                          '\r');
    }

    std::ptrdiff_t chunkSize = 0;
    std::stringstream ss;
    ss << std::hex << std::string(boost::begin(range), boost::begin(*chunk));
    ss >> chunkSize;
    chunkSize += 2;
    if (std::distance(boost::end(*chunk), end) < chunkSize) {
      break;
    }

    range = boost::make_iterator_range(boost::end(*chunk) + chunkSize, end);
    if (*(boost::begin(range) - 1) != '\n' ||
        *(boost::begin(range) - 2) != '\r') {
      throw ProtocolError("Failed to read chunk",
                          &*std::prev(boost::begin(range)), '\n');
    }
    chunk = boost::make_find_iterator(
        range, boost::first_finder("\r\n", boost::is_equal()));

    result = std::prev(boost::begin(range));
  }

  return result;
}

void HttpStreamClient::HandleNewMessages(
    const pt::ptime &time,
    const Buffer::const_iterator &begin,
    const Buffer::const_iterator &end,
    const TimeMeasurement::Milestones &timeMeasurement) {
  Assert(begin != end);

  auto range = boost::make_iterator_range(begin, std::next(end));

  if (m_pimpl->m_state == STATE_REQUEST_SENT) {
    const auto &header = boost::make_split_iterator(
        range, boost::first_finder("\r\n\r\n", boost::is_equal()));
    Assert(header != boost::split_iterator<Buffer::const_iterator>());
    AssertLe(4, std::distance(boost::end(*header), boost::end(range)));
    m_pimpl->HandleHeaders(header);
    m_pimpl->m_state = STATE_HEADER_RECEIVED;
    range =
        boost::make_iterator_range(boost::end(*header) + 4, boost::end(range));
    if (range.empty()) {
      return;
    }
  }

  auto chunk = boost::make_split_iterator(
      range, boost::first_finder("\r\n", boost::is_equal()));
  Assert(!chunk.eof());
  Assert(!chunk->empty());

  for (;;) {
    std::ptrdiff_t chunkSize = 0;
    std::stringstream ss;
    ss << std::hex << boost::copy_range<std::string>(*chunk);
    ss >> chunkSize;
    chunkSize += 2;

    const auto contentBegin = boost::end(*chunk) + 2;
    AssertLe(chunkSize, std::distance(contentBegin, boost::end(range)));
    if (chunkSize == 2) {
      if (chunkSize > std::distance(contentBegin, boost::end(range))) {
        throw ProtocolError("Failed to read last chunk",
                            &*std::prev(boost::end(*chunk) + chunkSize), 0);
      }
      Assert(contentBegin + chunkSize == boost::end(range));
      m_pimpl->m_state = STATE_NO_REQUEST;
      OnRequestComplete();
      return;
    }

    range =
        boost::make_iterator_range(contentBegin + chunkSize, boost::end(range));
    AssertEq('\n', *(boost::begin(range) - 1));
    AssertEq('\r', *(boost::begin(range) - 2));

    HandleNewSubMessages(time, contentBegin, boost::begin(range) - 2,
                         timeMeasurement);

    if (range.empty()) {
      break;
    }
    chunk = boost::make_split_iterator(
        range, boost::first_finder("\r\n", boost::is_equal()));
    Assert(!chunk.eof());
    Assert(!chunk->empty());
  }
}
