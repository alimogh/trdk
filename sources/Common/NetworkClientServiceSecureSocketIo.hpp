/*******************************************************************************
 *   Created: 2017/10/05 02:40:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "NetworkClientServiceSocketIo.hpp"

namespace trdk {
namespace Lib {

class NetworkClientServiceSecureSocketIo
    : public NetworkClientServiceSocketIo<
          boost::asio::ssl::stream<boost::asio::ip::tcp::socket &>> {
 public:
  typedef NetworkClientServiceSocketIo<
      boost::asio::ssl::stream<boost::asio::ip::tcp::socket &>>
      Base;

 public:
  explicit NetworkClientServiceSecureSocketIo(boost::asio::io_service &);
  virtual ~NetworkClientServiceSecureSocketIo() override = default;

 public:
  virtual boost::system::error_code Connect(
      const boost::asio::ip::tcp::resolver::query &&) override;

 protected:
  virtual Stream &GetStream() override { return m_stream; }
  virtual const Stream &GetStream() const override { return m_stream; }

 private:
  boost::asio::ssl::context m_context;
  Stream m_stream;
};
}
}