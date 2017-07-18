/**************************************************************************
 *   Created: 2016/08/24 13:30:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "NetworkStreamClient.hpp"
#include "NetworkClientServiceIo.hpp"
#include "NetworkStreamClientService.hpp"
#include "SysError.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace io = boost::asio;
namespace ip = io::ip;
namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

NetworkStreamClient::Exception::Exception(const char *what) noexcept
    : Lib::Exception(what) {}

NetworkStreamClient::ConnectError::ConnectError(const char *what) noexcept
    : Exception(what) {}

NetworkStreamClient::ProtocolError::ProtocolError(const char *what,
                                                  const char *bufferAddress,
                                                  char expectedByte) noexcept
    : Exception(what),
      m_bufferAddress(bufferAddress),
      m_expectedByte(expectedByte) {}
const char *NetworkStreamClient::ProtocolError::GetBufferAddress() const {
  return m_bufferAddress;
}
char NetworkStreamClient::ProtocolError::GetExpectedByte() const {
  return m_expectedByte;
}

class NetworkStreamClient::Implementation : private boost::noncopyable {
 public:
  NetworkStreamClient &m_self;

  NetworkStreamClientService &m_service;
  io::ip::tcp::socket m_socket;

  std::pair<Buffer, Buffer> m_buffer;
  BufferMutex m_bufferMutex;

  size_t m_numberOfReceivedBytes;

  explicit Implementation(NetworkStreamClientService &service,
                          NetworkStreamClient &self)
      : m_self(self),
        m_service(service),
        m_socket(m_service.GetIo().GetService()),
        m_numberOfReceivedBytes(0) {}

  void StartRead(Buffer &activeBuffer,
                 size_t bufferStartOffset,
                 Buffer &nextBuffer) {
    AssertLt(bufferStartOffset, activeBuffer.size());

#ifdef DEV_VER
    std::fill(activeBuffer.begin() + bufferStartOffset, activeBuffer.end(),
              0xff);
#endif

    auto self = m_self.shared_from_this();
    using Handler =
        boost::function<void(const boost::system::error_code &, size_t)>;
    const Handler &handler = [self, &activeBuffer, bufferStartOffset,
                              &nextBuffer](
        const boost::system::error_code &error, size_t transferredBytes) {
      self->m_pimpl->OnReadCompleted(activeBuffer, bufferStartOffset,
                                     nextBuffer, error, transferredBytes);
    };

    try {
      io::async_read(m_socket,
                     io::buffer(activeBuffer.data() + bufferStartOffset,
                                activeBuffer.size() - bufferStartOffset),
                     io::transfer_at_least(1),
                     boost::bind(handler, io::placeholders::error,
                                 io::placeholders::bytes_transferred));
    } catch (const std::exception &ex) {
      boost::format message("Failed to start read: \"%1%\"");
      message % ex.what();
      throw Exception(message.str().c_str());
    }
  }

  void OnReadCompleted(Buffer &activeBuffer,
                       size_t bufferStartOffset,
                       Buffer &nextBuffer,
                       const boost::system::error_code &error,
                       size_t transferredBytes) {
    const auto &timeMeasurement = m_self.StartMessageMeasurement();
    const auto &now = m_self.GetCurrentTime();

    if (error) {
      OnConnectionError(error);
      return;
    } else if (!transferredBytes) {
      {
        boost::format message(
            "%1%Connection was gratefully closed."
            " Received %2$.02f %3%.");
        message % m_service.GetLogTag();
        const auto &stat = m_self.GetReceivedVerbouseStat();
        message % stat.first % stat.second;
        m_self.LogInfo(message.str().c_str());
      }
      m_service.OnDisconnect();
      return;
    }

    // Checking for a message at the buffer end which wasn't fully received.
    const auto &transferedBegin = activeBuffer.cbegin() + bufferStartOffset;
    const auto &transferedEnd = transferedBegin + transferredBytes;

    // To synchronizes fast data packets it should be stopped before 1st
    // StartRead and buffers size change. Also, if FindLastMessageLastByte
    // reads state that can be set in HandleNewMessages - this two
    // operations should synced too.
    const BufferLock bufferLock(m_bufferMutex);

    m_numberOfReceivedBytes += transferredBytes;

    Buffer::const_iterator lastMessageLastByte;
    try {
      lastMessageLastByte = m_self.FindLastMessageLastByte(
          activeBuffer.cbegin(), transferedBegin, transferedEnd);
    } catch (const trdk::Lib::NetworkStreamClient::ProtocolError &ex) {
      Dump(ex, activeBuffer.cbegin(), transferedEnd);
      throw Exception("Protocol error");
    }
    Assert(transferedBegin <= lastMessageLastByte);
    Assert(lastMessageLastByte <= transferedEnd);

    const auto bufferedSize = bufferStartOffset + transferredBytes;
    const auto unreceivedMessageLen =
        lastMessageLastByte == transferedEnd
            ? bufferedSize
            : transferedEnd - std::next(lastMessageLastByte);
    AssertLe(unreceivedMessageLen, activeBuffer.size());

    if (unreceivedMessageLen > 0) {
      const auto freeBufferSpace = activeBuffer.size() - bufferedSize;

      // At the end of the buffer located message start without end. That
      // means that the buffer is too small to receive all messages.

      if (unreceivedMessageLen >= bufferedSize) {
        AssertEq(unreceivedMessageLen, bufferedSize);
        // Buffer not too small to receive all messages - it too small
        // to receive one message.
        if (unreceivedMessageLen / 3 > freeBufferSpace) {
          const auto newSize = activeBuffer.size() * 2;
#ifndef _DEBUG
          {
            boost::format message(
                "%1%Receiving large message in %2$.02f kilobytes..."
                " To optimize reading buffer 0x%3% will"
                " be increased: %4$.02f -> %5$.02f kilobytes."
                " Total received volume: %6$.02f %7%.");
            message % m_service.GetLogTag() %
                (double(unreceivedMessageLen) / 1024) % &activeBuffer %
                (double(activeBuffer.size()) / 1024) % (double(newSize) / 1024);
            const auto &stat = m_self.GetReceivedVerbouseStat();
            message % stat.first % stat.second;
            m_self.LogWarn(message.str());
          }
#endif
          if (newSize > (1024 * 1024) * 20) {
            throw Exception("The maximum buffer size is exceeded.");
          }
          activeBuffer.resize(newSize);
          nextBuffer.resize(newSize);
          // To the active buffer added more space at the end, so it
          // can continue to receive the current message in the active
          // buffer.
        }
        StartRead(activeBuffer, bufferStartOffset + transferredBytes,
                  nextBuffer);
        // Have no fully received messages in this buffer.
        return;
      }

      if (freeBufferSpace == 0) {
        nextBuffer.clear();
        const auto newSize = activeBuffer.size() * 2;
#ifndef _DEBUG
        {
          boost::format message(
              "%1%Increasing buffer 0x%2% size:"
              " %3$.02f -> %4$.02f kilobytes."
              " Total received volume: %5$.02f %6%.");
          message % m_service.GetLogTag() % &nextBuffer %
              (double(activeBuffer.size()) / 1024) % (double(newSize) / 1024);
          const auto &stat = m_self.GetReceivedVerbouseStat();
          message % stat.first % stat.second;
          m_self.LogDebug(message.str());
        }
#endif
        nextBuffer.resize(newSize);
      }

#ifndef _DEBUG
      if (unreceivedMessageLen >= 10 * 1024) {
        boost::format message(
            "%1%Restoring buffer content in %2$.02f kilobytes"
            " to continue to receive message..."
            " Total received volume: %3$.02f %4%.");
        message % m_service.GetLogTag() % (double(unreceivedMessageLen) / 1024);
        const auto &stat = m_self.GetReceivedVerbouseStat();
        message % stat.first % stat.second;
        m_self.LogDebug(message.str());
      }
#endif
      AssertGe(nextBuffer.size(), unreceivedMessageLen);
      std::copy(transferedEnd - unreceivedMessageLen, transferedEnd,
                nextBuffer.begin());
    }

    StartRead(nextBuffer, unreceivedMessageLen, activeBuffer);

    try {
      m_self.HandleNewMessages(now, activeBuffer.cbegin(), lastMessageLastByte,
                               timeMeasurement);
    } catch (const trdk::Lib::NetworkStreamClient::ProtocolError &ex) {
      Dump(ex, activeBuffer.cbegin(), lastMessageLastByte);
      throw Exception("Protocol error");
    }

    // activeBuffer.size() may be greater if handler of prev thread raised
    // exception after nextBuffer has been resized.
    if (activeBuffer.size() <= nextBuffer.size()) {
      activeBuffer.clear();
      activeBuffer.resize(nextBuffer.size());
    }
  }

  void OnConnectionError(const boost::system::error_code &error) {
    Assert(error);
    m_socket.close();
    {
      boost::format message(
          "%1%Connection to server closed by error:"
          " \"%2%\", (network error: \"%3%\")."
          " Received %4$.02f %5%.");
      message % m_service.GetLogTag() % SysError(error.value()) % error;
      const auto &stat = m_self.GetReceivedVerbouseStat();
      message % stat.first % stat.second;
      m_self.LogError(message.str());
    }
    m_service.OnDisconnect();
  }

  void Dump(const NetworkStreamClient::ProtocolError &ex,
            const Buffer::const_iterator &begin,
            const Buffer::const_iterator &end) const {
    std::ostringstream ss;
    ss << m_service.GetLogTag() << "Protocol error: \"" << ex << "\".";

    ss << " Active buffer: [ ";
    Assert(&*begin < ex.GetBufferAddress());
    Assert(ex.GetBufferAddress() <= &*(end));
    Assert(ex.GetBufferAddress() < &*(end));
    for (auto it = begin; it < end; ++it) {
      const bool isHighlighted = &*it == ex.GetBufferAddress();
      if (isHighlighted) {
        ss << '<';
      }
      ss << std::hex << std::setfill('0') << std::setw(2) << (*it & 0xff);
      if (isHighlighted) {
        ss << '>';
      }
      ss << ' ';
    }
    ss << "].";

    ss << " Expected byte: 0x" << std::hex << std::setfill('0') << std::setw(2)
       << (ex.GetExpectedByte() & 0xff) << '.';

    m_self.LogError(ss.str());
  }
};

NetworkStreamClient::NetworkStreamClient(NetworkStreamClientService &service,
                                         const std::string &host,
                                         size_t port)
    : m_pimpl(new Implementation(service, *this)) {
  try {
    const ip::tcp::resolver::query query(
        host, boost::lexical_cast<std::string>(port));

    {
      boost::system::error_code error;
      io::connect(m_pimpl->m_socket,
                  ip::tcp::resolver(m_pimpl->m_service.GetIo().GetService())
                      .resolve(query),
                  error);
      if (error) {
        boost::format errorText("\"%1%\" (network error: \"%2%\")");
        errorText % SysError(error.value()) % error;
        throw ConnectError(errorText.str().c_str());
      }
    }

  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }
}

NetworkStreamClient::~NetworkStreamClient() {
  try {
    m_pimpl->m_service.OnClientDestroy();
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

size_t NetworkStreamClient::GetNumberOfReceivedBytes() const {
  return m_pimpl->m_numberOfReceivedBytes;
}

NetworkStreamClientService &NetworkStreamClient::GetService() {
  return m_pimpl->m_service;
}
const NetworkStreamClientService &NetworkStreamClient::GetService() const {
  return const_cast<NetworkStreamClient *>(this)->GetService();
}

void NetworkStreamClient::Start() {
  AssertEq(0, m_pimpl->m_buffer.first.size());
  AssertEq(0, m_pimpl->m_buffer.second.size());

  {
    const auto timeoutMilliseconds = 15 * 1000;
#ifdef BOOST_WINDOWS
    const DWORD timeout = timeoutMilliseconds;
#else
    const timeval timeout = {timeoutMilliseconds / 1000};
#endif
    auto setsockoptResult =
        setsockopt(m_pimpl->m_socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char *>(&timeout), sizeof(timeout));
    if (setsockoptResult) {
      boost::format message("%1%Failed to set SO_RCVTIMEO: \"%2%\".");
      message % GetService().GetLogTag() % setsockoptResult;
      LogError(message.str());
    }
    setsockoptResult =
        setsockopt(m_pimpl->m_socket.native_handle(), SOL_SOCKET, SO_SNDTIMEO,
                   reinterpret_cast<const char *>(&timeout), sizeof(timeout));
    if (setsockoptResult) {
      boost::format message("%1%Failed to set SO_SNDTIMEO: \"%2%\".");
      message % GetService().GetLogTag() % setsockoptResult;
      LogError(message.str());
    }
  }

  OnStart();

#if DEV_VER
  const size_t initiaBufferSize = 256;
#else
  const size_t initiaBufferSize = (1024 * 1024) * 2;
#endif
  m_pimpl->m_buffer.first.resize(initiaBufferSize);
  m_pimpl->m_buffer.second.resize(m_pimpl->m_buffer.first.size());
#ifdef DEV_VER
  std::fill(m_pimpl->m_buffer.first.begin(), m_pimpl->m_buffer.first.end(),
            0xff);
  std::fill(m_pimpl->m_buffer.second.begin(), m_pimpl->m_buffer.second.end(),
            0xff);
#endif

  m_pimpl->StartRead(m_pimpl->m_buffer.first, 0, m_pimpl->m_buffer.second);
}

void NetworkStreamClient::Stop() {
  if (!m_pimpl->m_socket.is_open()) {
    return;
  }
  {
    boost::format message("%1%Closing connection...");
    message % GetService().GetLogTag();
    LogInfo(message.str().c_str());
  }
  m_pimpl->m_socket.shutdown(io::ip::tcp::socket::shutdown_both);
  m_pimpl->m_socket.close();
}

bool NetworkStreamClient::CheckResponceSynchronously(
    const char *actionName,
    const char *expectedResponse,
    const char *errorResponse) {
  Assert(expectedResponse);
  Assert(strlen(expectedResponse));
  Assert(actionName);
  Assert(strlen(actionName));
  Assert(!errorResponse || strlen(errorResponse));

  // Available only before asynchronous mode start:
  AssertEq(0, m_pimpl->m_buffer.first.size());
  AssertEq(0, m_pimpl->m_buffer.second.size());

  const size_t expectedResponseSize = strlen(expectedResponse);

  std::vector<char> serverResponse(
      errorResponse ? std::max(strlen(errorResponse), expectedResponseSize)
                    : expectedResponseSize);
  boost::system::error_code error;
  const auto &serverResponseSize =
      io::read(m_pimpl->m_socket, io::buffer(serverResponse),
               io::transfer_at_least(1), error);
  AssertLe(serverResponseSize, serverResponse.size());

  if (error) {
    boost::format message(
        "Failed to read %1% response: \"%2%\", (network error: \"%3%\")");
    message % actionName % SysError(error.value()) % error;
    throw Exception(message.str().c_str());
  }

  if (!serverResponseSize) {
    boost::format message("Connection closed by server at %1%");
    message % actionName;
    throw Exception(message.str().c_str());
  }

  if (errorResponse && strlen(errorResponse) == serverResponseSize &&
      !memcmp(&serverResponse[0], errorResponse, serverResponseSize)) {
    return false;
  }

  if (serverResponseSize != expectedResponseSize ||
      memcmp(&serverResponse[0], expectedResponse, serverResponseSize)) {
    {
      serverResponse[serverResponseSize] = 0;
      boost::format logMessage(
          "%1%Unexpected %2% response from server (size: %3% bytes)"
          ": \"%4%\".");
      logMessage % GetService().GetLogTag() % actionName % serverResponseSize %
          &serverResponse[0];
      LogError(logMessage.str());
    }
    boost::format logMessage("Unexpected %1% response from server");
    logMessage % actionName;
    throw Exception(logMessage.str().c_str());
  }

  return true;
}

void NetworkStreamClient::Send(std::string &&message) {
  Assert(!message.empty());

  // Available only after asynchronous mode start:
  AssertLt(0, m_pimpl->m_buffer.first.size());
  AssertLt(0, m_pimpl->m_buffer.second.size());

  const auto &self = shared_from_this();
  const auto &messageCopy = boost::make_shared<std::string>(std::move(message));
  const boost::function<void(const boost::system::error_code &)> &callback =
      [self, messageCopy](const boost::system::error_code &error) {
        if (error) {
          self->m_pimpl->OnConnectionError(error);
        }
      };

  try {
    io::async_write(m_pimpl->m_socket, io::buffer(*messageCopy),
                    boost::bind(callback, io::placeholders::error));
  } catch (const std::exception &ex) {
    boost::format error("Failed to write to socket: \"%1%\"");
    error % ex.what();
    throw Exception(error.str().c_str());
  }
}

void NetworkStreamClient::SendSynchronously(const std::string &message,
                                            const char *requestName) {
  Assert(requestName);
  Assert(strlen(requestName));

  // Available only before asynchronous mode start:
  AssertEq(0, m_pimpl->m_buffer.first.size());
  AssertEq(0, m_pimpl->m_buffer.second.size());

  boost::system::error_code error;
  const auto size = io::write(m_pimpl->m_socket, io::buffer(message), error);

  if (error || size != message.size()) {
    {
      boost::format logMessage(
          "%1%Failed to send %2%: \"%3%\", (network error: \"%4%\")."
          " Message size: %5% bytes, sent: %6% bytes.");
      logMessage % GetService().GetLogTag() % requestName %
          SysError(error.value()) % error % message.size() % size;
      LogError(logMessage.str());
    }
    boost::format exceptionMessage("Failed to send %1%");
    exceptionMessage % requestName;
    throw Exception(exceptionMessage.str().c_str());
  }
}

bool NetworkStreamClient::RequestSynchronously(const std::string &message,
                                               const char *requestName,
                                               const char *expectedResponse,
                                               const char *errorResponse) {
  SendSynchronously(message, requestName);
  return CheckResponceSynchronously(requestName, expectedResponse,
                                    errorResponse);
}

std::pair<double, std::string> NetworkStreamClient::GetReceivedVerbouseStat()
    const {
  if (m_pimpl->m_numberOfReceivedBytes > ((1024 * 1024) * 1024)) {
    return std::make_pair(
        double(m_pimpl->m_numberOfReceivedBytes) / ((1024 * 1024) * 1024),
        "gigabytes");
  } else if (m_pimpl->m_numberOfReceivedBytes > (1024 * 1024)) {
    return std::make_pair(
        double(m_pimpl->m_numberOfReceivedBytes) / (1024 * 1024), "megabytes");
  } else {
    return std::make_pair(double(m_pimpl->m_numberOfReceivedBytes) / 1024,
                          "kilobytes");
  }
}

NetworkStreamClient::BufferLock NetworkStreamClient::LockDataExchange() {
  return BufferLock(m_pimpl->m_bufferMutex);
}
