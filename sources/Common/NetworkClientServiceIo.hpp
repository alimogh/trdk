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
  boost::asio::io_service& GetService() { return m_service; }

 private:
  boost::asio::io_service m_service;
};
}
}
