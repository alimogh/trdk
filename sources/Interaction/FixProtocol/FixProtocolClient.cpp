/*******************************************************************************
 *   Created: 2017/09/20 19:51:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "FixProtocolClient.hpp"
#include "FixProtocolHandler.hpp"
#include "FixProtocolIncomingMessagesFabric.hpp"
#include "FixProtocolMessage.hpp"
#include "FixProtocolOutgoingMessages.hpp"
#include "FixProtocolSettings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Interaction::FixProtocol;

namespace pt = boost::posix_time;
namespace fix = trdk::Interaction::FixProtocol;

////////////////////////////////////////////////////////////////////////////////

namespace {

class Connection : public NetworkStreamClient {
 public:
  typedef NetworkStreamClient Base;

 public:
  explicit Connection(Client &service)
      : Base(service,
             service.GetHandler().GetSettings().host,
             service.GetHandler().GetSettings().port) {}

  virtual ~Connection() override = default;

 protected:
  Handler &GetHandler() { return GetService().GetHandler(); }
  const Handler &GetHandler() const { return GetService().GetHandler(); }
  const Context &GetContext() const { return GetHandler().GetContext(); }
  Context &GetContext() { return GetHandler().GetContext(); }
  const fix::Settings &GetSettings() { return GetHandler().GetSettings(); }
  ModuleEventsLog &GetLog() const { return GetHandler().GetLog(); }

 protected:
  virtual Milestones StartMessageMeasurement() const override {
    return GetContext().StartStrategyTimeMeasurement();
  }
  virtual pt::ptime GetCurrentTime() const override {
    return GetContext().GetCurrentTime();
  }
  virtual void LogDebug(const std::string &message) const override {
    GetLog().Debug(message.c_str());
  }
  virtual void LogInfo(const std::string &message) const override {
    GetLog().Info(message.c_str());
  }
  virtual void LogWarn(const std::string &message) const override {
    GetLog().Warn(message.c_str());
  }
  virtual void LogError(const std::string &message) const override {
    GetLog().Error(message.c_str());
  }
  virtual Client &GetService() override {
    return *boost::polymorphic_downcast<Client *>(&Base::GetService());
  }
  virtual const Client &GetService() const override {
    return *boost::polymorphic_downcast<const Client *>(&Base::GetService());
  }

 protected:
  virtual void OnStart() override {
    Assert(!GetHandler().IsAuthorized());
    SendSynchronously(
        Outgoing::Logon(GetHandler().GetStandardOutgoingHeader()).Export(SOH),
        "Logon");
    const auto &responceContent = ReceiveSynchronously("Logon", 256);
    Incoming::Factory::Create(responceContent.cbegin(), responceContent.cend(),
                              *GetSettings().policy)
        ->Handle(GetHandler(), *this, Milestones());
    if (!GetHandler().IsAuthorized()) {
      ConnectError("Failed to authorize");
    }
  }

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
      const Buffer::const_iterator &transferedBegin,
      const Buffer::const_iterator &bufferEnd) const override {
    const Buffer::const_reverse_iterator bufferREnd(bufferBegin);
    const Buffer::const_reverse_iterator transferedREnd(transferedBegin);
    Buffer::const_reverse_iterator rbegin(bufferEnd);
    const Buffer::const_reverse_iterator rend(bufferBegin);

    rbegin = std::find(rbegin, transferedREnd, SOH);
    if (rbegin == transferedREnd) {
      return bufferEnd;
    }
    ++rbegin;

    for (;;) {
      const auto checkSumFieldBegin = std::find(rbegin, bufferREnd, SOH);
      if (checkSumFieldBegin == bufferREnd) {
        return bufferEnd;
      }

      const auto &fieldLen = -std::distance(checkSumFieldBegin, rbegin);

      // |10=
      static const int32_t checkSumTagMatch = 1026568449;
      if (fieldLen >= sizeof(checkSumTagMatch) - 1 &&
          reinterpret_cast<decltype(checkSumTagMatch) &>(*checkSumFieldBegin) ==
              checkSumTagMatch) {
        return rbegin.base();
      }

      if (checkSumFieldBegin < transferedREnd) {
        return bufferEnd;
      }

      rbegin = checkSumFieldBegin;
    }
  }

  //! Handles messages in the buffer.
  /** Called under lock. This range has one or more messages.
    */
  virtual void HandleNewMessages(const pt::ptime &,
                                 const Buffer::const_iterator &begin,
                                 const Buffer::const_iterator &end,
                                 const Milestones &delayMeasurement) override {
    const auto &bufferEnd = std::next(end);
    for (auto messageBegin = begin; messageBegin < bufferEnd;) {
      const auto &message = Incoming::Factory::Create(messageBegin, bufferEnd,
                                                      *GetSettings().policy);
      message->Handle(GetHandler(), *this, delayMeasurement);
      messageBegin = message->GetMessageEnd();
      Assert(messageBegin <= bufferEnd);
    }
  }
};
}

////////////////////////////////////////////////////////////////////////////////

Client::~Client() noexcept {
  try {
    Stop();
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

pt::ptime Client::GetCurrentTime() const {
  return GetHandler().GetContext().GetCurrentTime();
}

std::unique_ptr<NetworkStreamClient> Client::CreateClient() {
  return boost::make_unique<Connection>(*this);
}

void Client::LogDebug(const char *message) const {
  GetHandler().GetLog().Debug(message);
}

void Client::LogInfo(const std::string &message) const {
  GetHandler().GetLog().Info(message.c_str());
}

void Client::LogError(const std::string &message) const {
  GetHandler().GetLog().Error(message.c_str());
}

void Client::OnConnectionRestored() { m_handler.OnConnectionRestored(); }

void Client::OnStopByError(const std::string &) {}

void Client::Send(const Outgoing::Message &message) {
  InvokeClient<Connection>([&message](Connection &connection) {
    connection.Send(message.Export(SOH));
  });
}

////////////////////////////////////////////////////////////////////////////////
