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
#include "FixProtocolMarketDataSource.hpp"
#include "FixProtocolSecurity.hpp"
#include "Common/NetworkStreamClient.hpp"

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
             service.GetSource().GetSettings().host,
             service.GetSource().GetSettings().port) {}

  virtual ~Connection() override = default;

 protected:
  const fix::MarketDataSource &GetSource() const {
    return GetService().GetSource();
  }
  const Context &GetContext() const { return GetSource().GetContext(); }
  fix::MarketDataSource::Log &GetLog() const { return GetSource().GetLog(); }

 protected:
  virtual Milestones StartMessageMeasurement() const override {
    return GetContext().StartStrategyTimeMeasurement();
  }
  virtual pt::ptime GetCurrentTime() const override {
    return GetSource().GetContext().GetCurrentTime();
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
  virtual void OnStart() override {}

  //! Find message end by reverse iterators.
  /** Called under lock.
    * @param[in] bufferBegin     Buffer begin.
    * @param[in] transferedBegin Last operation transfered begin.
    * @param[in] bufferEnd       Buffer end.
    * @return Last byte of a message, or bufferEnd if the range
    *         doesn't include message end.
    */
  virtual Buffer::const_iterator FindLastMessageLastByte(
      const Buffer::const_iterator &,
      const Buffer::const_iterator &,
      const Buffer::const_iterator &bufferEnd) const override {
    return bufferEnd;
  }

  //! Handles messages in the buffer.
  /** Called under lock. This range has one or more messages.
    */
  virtual void HandleNewMessages(const boost::posix_time::ptime &,
                                 const Buffer::const_iterator &,
                                 const Buffer::const_iterator &,
                                 const Milestones &) override {}
};
}

////////////////////////////////////////////////////////////////////////////////

Client::Client(const std::string &name, fix::MarketDataSource &source)
    : Base(name), m_source(source) {}

Client::~Client() noexcept {
  try {
    Stop();
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

pt::ptime Client::GetCurrentTime() const {
  return GetSource().GetContext().GetCurrentTime();
}

std::unique_ptr<NetworkStreamClient> Client::CreateClient() {
  return boost::make_unique<Connection>(*this);
}

void Client::LogDebug(const char *message) const {
  GetSource().GetLog().Debug(message);
}

void Client::LogInfo(const std::string &message) const {
  GetSource().GetLog().Info(message.c_str());
}

void Client::LogError(const std::string &message) const {
  GetSource().GetLog().Error(message.c_str());
}

void Client::OnConnectionRestored() { GetSource().ResubscribeToSecurities(); }

void Client::OnStopByError(const std::string &) {}

////////////////////////////////////////////////////////////////////////////////
