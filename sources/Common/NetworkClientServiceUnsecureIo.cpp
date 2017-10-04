/*******************************************************************************
 *   Created: 2017/10/05 01:45:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once
#include "Prec.hpp"
#include "NetworkClientServiceUnsecureIo.hpp"

using namespace trdk::Lib;

namespace io = boost::asio;
namespace ip = io::ip;

bool NetworkClientServiceUnsecureIo::IsOpen() const {
  return m_socket.is_open();
}

ip::tcp::socket::native_handle_type
NetworkClientServiceUnsecureIo::GetNativeHandle() {
  return m_socket.native_handle();
}

boost::system::error_code NetworkClientServiceUnsecureIo::Connect(
    const ip::tcp::resolver::query &&query) {
  boost::system::error_code error;
  io::connect(m_socket,
              ip::tcp::resolver(GetService()).resolve(std::move(query)), error);
  return error;
}

void NetworkClientServiceUnsecureIo::Close() { m_socket.close(); }

void NetworkClientServiceUnsecureIo::Shutdown(
    const io::socket_base::shutdown_type &type) {
  m_socket.shutdown(type);
}

std::pair<boost::system::error_code, size_t>
NetworkClientServiceUnsecureIo::Write(const io::const_buffers_1 &&buffer) {
  boost::system::error_code error;
  const auto &size = io::write(m_socket, std::move(buffer), error);
  return {std::move(error), std::move(size)};
}

std::pair<boost::system::error_code, size_t>
NetworkClientServiceUnsecureIo::Write(
    const std::vector<io::const_buffer> &&buffer) {
  boost::system::error_code error;
  const auto &size = io::write(m_socket, std::move(buffer), error);
  return {std::move(error), std::move(size)};
}

void NetworkClientServiceUnsecureIo::StartAsyncWrite(
    const io::const_buffers_1 &&buffer,
    const boost::function<void(const boost::system::error_code &)> &&callback) {
  io::async_write(m_socket, std::move(buffer),
                  boost::bind(std::move(callback), io::placeholders::error));
}

void NetworkClientServiceUnsecureIo::StartAsyncWrite(
    const std::vector<io::const_buffer> &&buffer,
    const boost::function<void(const boost::system::error_code &)> &&callback) {
  io::async_write(m_socket, std::move(buffer),
                  boost::bind(std::move(callback), io::placeholders::error));
}

std::pair<boost::system::error_code, size_t>
NetworkClientServiceUnsecureIo::Read(
    const io::mutable_buffers_1 &&buffer,
    const io::detail::transfer_at_least_t &&transferAtLeast) {
  boost::system::error_code error;
  const auto &size =
      io::read(m_socket, std::move(buffer), transferAtLeast, error);
  return {std::move(error), std::move(size)};
}

void NetworkClientServiceUnsecureIo::StartAsyncRead(
    const io::mutable_buffers_1 &&buffer,
    const io::detail::transfer_at_least_t &&transferAtLeast,
    const boost::function<void(const boost::system::error_code &, size_t)>
        &&callback) {
  io::async_read(m_socket, std::move(buffer), std::move(transferAtLeast),
                 boost::bind(std::move(callback), io::placeholders::error,
                             io::placeholders::bytes_transferred));
}
