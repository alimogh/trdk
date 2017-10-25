/*******************************************************************************
 *   Created: 2017/10/05 01:41:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "NetworkClientServiceSocketIo.hpp"

namespace trdk {
namespace Lib {

class NetworkClientServiceUnsecureSocketIo
    : public NetworkClientServiceSocketIo<boost::asio::ip::tcp::socket> {
 public:
  typedef NetworkClientServiceSocketIo<boost::asio::ip::tcp::socket> Base;

 public:
  NetworkClientServiceUnsecureSocketIo(boost::asio::io_service &service)
      : Base(service) {}

 protected:
  virtual boost::asio::ip::tcp::socket &GetStream() override {
    return GetSocket();
  }
  virtual const boost::asio::ip::tcp::socket &GetStream() const override {
    return GetSocket();
  }
};
}
}