/*******************************************************************************
 *   Created: 2017/09/19 19:55:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MarketDataSource.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::FixProtocol;

namespace fix = trdk::Interaction::FixProtocol;
namespace gr = boost::gregorian;

fix::MarketDataSource::MarketDataSource(size_t index,
                                        Context &context,
                                        const std::string &instanceName,
                                        const IniSectionRef &conf)
    : Base(index, context, instanceName),
      m_settings(conf),
      m_client("Prices", *this) {
  m_settings.Log(GetLog());
  m_settings.Validate();
}

void fix::MarketDataSource::Connect(const IniSectionRef &) {
  GetLog().Debug("Connecting to the stream...");
  m_client.Connect();
  GetLog().Info("Connected to the stream.");
}

void fix::MarketDataSource::SubscribeToSecurities() {}

void fix::MarketDataSource::ResubscribeToSecurities() {}

Security &fix::MarketDataSource::CreateNewSecurityObject(const Symbol &) {
  throw Exception("Failed to create security object");
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::MarketDataSource> CreateMarketDataSource(
    size_t index,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<fix::MarketDataSource>(index, context, instanceName,
                                                   configuration);
}

////////////////////////////////////////////////////////////////////////////////
