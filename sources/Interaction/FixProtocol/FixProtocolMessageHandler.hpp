/*******************************************************************************
 *   Created: 2017/09/24 03:50:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Interaction {
namespace FixProtocol {
class MessageHandler : private boost::noncopyable {
 public:
  virtual ~MessageHandler() = default;

 public:
  virtual void OnLogon(const Incoming::Logon &) = 0;

  virtual void OnHeartbeat(const Incoming::Heartbeat &) = 0;
  virtual void OnTestRequest(const Incoming::TestRequest &) = 0;
};
}
}
}