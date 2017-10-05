/*******************************************************************************
 *   Created: 2017/10/05 02:41:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "NetworkClientServiceSecureSocketIo.hpp"

using namespace trdk::Lib;

namespace io = boost::asio;
namespace ip = io::ip;
namespace ssl = io::ssl;

NetworkClientServiceSecureSocketIo::NetworkClientServiceSecureSocketIo(
    boost::asio::io_service &service)
    : Base(service),
      m_context(ssl::context::sslv23),
      m_stream(GetSocket(), m_context) {}

boost::system::error_code NetworkClientServiceSecureSocketIo::Connect(
    const ip::tcp::resolver::query &&query) {
  const auto &error = Base::Connect(std::move(query));
  if (error) {
    return error;
  }
  GetSocket().set_option(ip::tcp::no_delay(true));
  m_stream.set_verify_mode(ssl::verify_none);
  m_stream.handshake(Stream::client);
  return error;
}
