/**************************************************************************
 *   Created: 2016/08/26 07:57:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk {
namespace Lib {

class NetworkClientServiceIo : private boost::noncopyable {
 public:
  explicit NetworkClientServiceIo(boost::asio::io_service &service)
      : m_service(service) {}
  virtual ~NetworkClientServiceIo() = default;

 public:
  virtual bool IsOpen() const = 0;

 public:
  boost::asio::io_service &GetService() { return m_service; }
  virtual boost::asio::ip::tcp::socket::native_handle_type
  GetNativeHandle() = 0;

 public:
  virtual boost::system::error_code Connect(
      const boost::asio::ip::tcp::resolver::query &&) = 0;
  virtual void Close() = 0;
  virtual void Shutdown(const boost::asio::socket_base::shutdown_type &) = 0;

 public:
  virtual std::pair<boost::system::error_code, size_t> Write(
      const std::vector<boost::asio::const_buffer> &&) = 0;
  virtual std::pair<boost::system::error_code, size_t> Write(
      const boost::asio::const_buffers_1 &&) = 0;
  virtual void StartAsyncWrite(
      const std::vector<boost::asio::const_buffer> &&,
      const boost::function<void(const boost::system::error_code &)> &&) = 0;
  virtual void StartAsyncWrite(
      const boost::asio::const_buffers_1 &&,
      const boost::function<void(const boost::system::error_code &)> &&) = 0;

 public:
  virtual std::pair<boost::system::error_code, size_t> Read(
      const boost::asio::mutable_buffers_1 &&,
      const boost::asio::detail::transfer_at_least_t &&) = 0;
  virtual void StartAsyncRead(
      const boost::asio::mutable_buffers_1 &&,
      const boost::asio::detail::transfer_at_least_t &&,
      const boost::function<void(const boost::system::error_code &, size_t)>
          &&) = 0;

 private:
  boost::asio::io_service &m_service;
};
}
}
