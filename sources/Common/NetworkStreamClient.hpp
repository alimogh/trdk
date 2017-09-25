/**************************************************************************
 *   Created: 2016/08/24 13:29:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Exception.hpp"
#include "TimeMeasurement.hpp"
#include "Fwd.hpp"
#include <boost/asio/buffer.hpp>

namespace trdk {
namespace Lib {

class NetworkStreamClient
    : private boost::noncopyable,
      public boost::enable_shared_from_this<trdk::Lib::NetworkStreamClient> {
 public:
  class Exception : public trdk::Lib::Exception {
   public:
    explicit Exception(const char *what) noexcept;
  };

  class ConnectError : public trdk::Lib::NetworkStreamClient::Exception {
   public:
    explicit ConnectError(const char *what) noexcept;
  };

  class ProtocolError : public trdk::Lib::NetworkStreamClient::Exception {
   public:
    explicit ProtocolError(const char *what,
                           const char *bufferAddress,
                           char expectedByte) noexcept;

   public:
    const char *GetBufferAddress() const;
    char GetExpectedByte() const;

   private:
    const char *m_bufferAddress;
    char m_expectedByte;
  };

 protected:
  typedef std::vector<char> Buffer;

  typedef boost::recursive_mutex BufferMutex;
  typedef BufferMutex::scoped_lock BufferLock;

 public:
  explicit NetworkStreamClient(trdk::Lib::NetworkStreamClientService &,
                               const std::string &host,
                               size_t port);
  virtual ~NetworkStreamClient();

 public:
  void Start();
  void Stop();

 protected:
  virtual trdk::Lib::TimeMeasurement::Milestones StartMessageMeasurement()
      const = 0;
  virtual boost::posix_time::ptime GetCurrentTime() const = 0;
  virtual void LogDebug(const std::string &message) const = 0;
  virtual void LogInfo(const std::string &message) const = 0;
  virtual void LogWarn(const std::string &message) const = 0;
  virtual void LogError(const std::string &message) const = 0;

  virtual void OnStart() = 0;

  //! Find message end by reverse iterators.
  /** Called under lock.
    * @param[in] bufferBegin     Buffer begin.
    * @param[in] transferedBegin Last operation transfered begin.
    * @param[in] bufferEnd       Buffer end.
    * @return Last byte of a message, or bufferEnd if the range
    *         doesn't include message end.
    */
  virtual Buffer::const_iterator FindLastMessageLastByte(
      const Buffer::const_iterator &bufferBegin,
      const Buffer::const_iterator &transferedEnd,
      const Buffer::const_iterator &bufferEnd) const = 0;

  //! Handles messages in the buffer.
  /** Called under lock. This range has one or more messages.
    */
  virtual void HandleNewMessages(
      const boost::posix_time::ptime &time,
      const Buffer::const_iterator &begin,
      const Buffer::const_iterator &end,
      const trdk::Lib::TimeMeasurement::Milestones &) = 0;

 protected:
  //! Returns number of received bytes.
  /** Thread-safe only from HandleNewMessages call.
    */
  size_t GetNumberOfReceivedBytes() const;

  //! Returns number of received bytes.
  /** Thread-safe only from HandleNewMessages call.
    *
    * @return Pair where 1-st value - value, second - unit name.
    */
  std::pair<double, std::string> GetReceivedVerbouseStat() const;

  virtual trdk::Lib::NetworkStreamClientService &GetService();
  virtual const trdk::Lib::NetworkStreamClientService &GetService() const;

  void Send(const std::vector<boost::asio::const_buffer> &&,
            const boost::function<void()> &&onOperaton—ompletion);
  void Send(const std::vector<char> &&);
  void Send(const std::string &&);
  void Send(const char *persistentBuffer, size_t len);

  bool CheckResponceSynchronously(const char *actionName,
                                  const char *expectedResponse,
                                  const char *errorResponse = nullptr);

  void SendSynchronously(const std::vector<char> &message,
                         const char *requestName);
  void SendSynchronously(const std::string &message, const char *requestName);
  std::vector<char> ReceiveSynchronously(const char *requestName, size_t size);

  bool RequestSynchronously(const std::string &message,
                            const char *requestName,
                            const char *expectedResponse,
                            const char *errorResponse = nullptr);

  //! Locks data acrimonious exchange (recursive mutex).
  BufferLock LockDataExchange();

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
