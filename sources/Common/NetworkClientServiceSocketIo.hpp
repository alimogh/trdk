/*******************************************************************************
 *   Created: 2017/10/05 14:59:09
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

template <typename StreamType>
class NetworkClientServiceSocketIo : public NetworkClientServiceIo {
 protected:
  typedef StreamType Stream;

 public:
  explicit NetworkClientServiceSocketIo(boost::asio::io_service &service)
      : NetworkClientServiceIo(service), m_socket(GetService()) {}
  virtual ~NetworkClientServiceSocketIo() override = default;

 public:
  virtual bool IsOpen() const override { return GetSocket().is_open(); }

  virtual boost::asio::ip::tcp::socket::native_handle_type GetNativeHandle()
      override {
    return GetSocket().native_handle();
  }

  virtual boost::system::error_code Connect(
      const boost::asio::ip::tcp::resolver::query &&query) override {
    boost::system::error_code error;
    boost::asio::connect(
        GetSocket(), ip::tcp::resolver(GetService()).resolve(std::move(query)),
        error);
    return error;
  }

  virtual void Close() override { GetSocket().close(); }

  virtual void Shutdown(
      const boost::asio::socket_base::shutdown_type &type) override {
    GetSocket().shutdown(type);
  }

  virtual std::pair<boost::system::error_code, size_t> Write(
      const boost::asio::const_buffers_1 &&buffer) override {
    boost::system::error_code error;
    const auto &size =
        boost::asio::write(GetStream(), std::move(buffer), error);
    return {std::move(error), std::move(size)};
  }
  virtual std::pair<boost::system::error_code, size_t> Write(
      const std::vector<boost::asio::const_buffer> &&buffer) override {
    boost::system::error_code error;
    const auto &size =
        boost::asio::write(GetStream(), std::move(buffer), error);
    return {std::move(error), std::move(size)};
  }
  virtual void StartAsyncWrite(
      const boost::asio::const_buffers_1 &&buffer,
      const boost::function<void(const boost::system::error_code &)> &&callback)
      override {
    boost::asio::async_write(
        GetStream(), std::move(buffer),
        boost::bind(std::move(callback), boost::asio::placeholders::error));
  }
  virtual void StartAsyncWrite(
      const std::vector<boost::asio::const_buffer> &&buffer,
      const boost::function<void(const boost::system::error_code &)> &&callback)
      override {
    boost::asio::async_write(
        GetStream(), std::move(buffer),
        boost::bind(std::move(callback), boost::asio::placeholders::error));
  }

  virtual std::pair<boost::system::error_code, size_t> Read(
      const boost::asio::mutable_buffers_1 &&buffer,
      const boost::asio::detail::transfer_at_least_t &&transferAtLeast)
      override {
    boost::system::error_code error;
    const auto &size = boost::asio::read(GetStream(), std::move(buffer),
                                         transferAtLeast, error);
    return {std::move(error), std::move(size)};
  }
  virtual void StartAsyncRead(
      const boost::asio::mutable_buffers_1 &&buffer,
      const boost::asio::detail::transfer_at_least_t &&transferAtLeast,
      const boost::function<void(const boost::system::error_code &, size_t)>
          &&callback) override {
    boost::asio::async_read(
        GetStream(), std::move(buffer), std::move(transferAtLeast),
        boost::bind(std::move(callback), boost::asio::placeholders::error,
                    io::placeholders::bytes_transferred));
  }

 protected:
  boost::asio::ip::tcp::socket &GetSocket() { return m_socket; }
  const boost::asio::ip::tcp::socket &GetSocket() const { return m_socket; }

  virtual Stream &GetStream() = 0;
  virtual const Stream &GetStream() const = 0;

 private:
  boost::asio::ip::tcp::socket m_socket;
};
}
}