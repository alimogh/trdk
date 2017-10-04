/*******************************************************************************
 *   Created: 2017/10/05 01:41:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "NetworkClientServiceIo.hpp"

namespace trdk {
namespace Lib {

class NetworkClientServiceUnsecureIo : public NetworkClientServiceIo {
 public:
  explicit NetworkClientServiceUnsecureIo(boost::asio::io_service &service)
      : NetworkClientServiceIo(service), m_socket(GetService()) {}
  virtual ~NetworkClientServiceUnsecureIo() override = default;

 public:
  virtual bool IsOpen() const override;
  virtual boost::asio::ip::tcp::socket::native_handle_type GetNativeHandle()
      override;

  virtual boost::system::error_code Connect(
      const boost::asio::ip::tcp::resolver::query &&) override;
  virtual void Close() override;
  virtual void Shutdown(
      const boost::asio::socket_base::shutdown_type &) override;

  virtual std::pair<boost::system::error_code, size_t> Write(
      const boost::asio::const_buffers_1 &&) override;
  virtual std::pair<boost::system::error_code, size_t> Write(
      const std::vector<boost::asio::const_buffer> &&) override;
  virtual void StartAsyncWrite(
      const boost::asio::const_buffers_1 &&,
      const boost::function<void(const boost::system::error_code &)> &&)
      override;
  virtual void StartAsyncWrite(
      const std::vector<boost::asio::const_buffer> &&,
      const boost::function<void(const boost::system::error_code &)> &&)
      override;

  virtual std::pair<boost::system::error_code, size_t> Read(
      const boost::asio::mutable_buffers_1 &&,
      const boost::asio::detail::transfer_at_least_t &&) override;
  virtual void StartAsyncRead(
      const boost::asio::mutable_buffers_1 &&,
      const boost::asio::detail::transfer_at_least_t &&transferAtLeast,
      const boost::function<void(const boost::system::error_code &, size_t)> &&)
      override;

 private:
  boost::asio::ip::tcp::socket m_socket;
};
}
}