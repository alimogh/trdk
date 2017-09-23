/*******************************************************************************
 *   Created: 2017/09/19 19:48:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Common/NetworkStreamClientService.hpp"

namespace trdk {
namespace Interaction {
namespace FixProtocol {

class Client : public Lib::NetworkStreamClientService {
 public:
  typedef Lib::NetworkStreamClientService Base;

 public:
  explicit Client(const std::string &name, MarketDataSource &);
  virtual ~Client() noexcept override;

 public:
  MarketDataSource &GetSource() { return m_source; }
  const MarketDataSource &GetSource() const {
    return const_cast<Client *>(this)->GetSource();
  }

 protected:
  virtual boost::posix_time::ptime GetCurrentTime() const override;

  virtual std::unique_ptr<trdk::Lib::NetworkStreamClient> CreateClient()
      override;

  virtual void LogDebug(const char *) const override;
  virtual void LogInfo(const std::string &) const override;
  virtual void LogError(const std::string &) const override;

  virtual void OnConnectionRestored() override;
  virtual void OnStopByError(const std::string &) override;

 private:
  MarketDataSource &m_source;
};
}
}
}