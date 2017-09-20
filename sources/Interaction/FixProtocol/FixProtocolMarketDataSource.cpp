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
#include "FixProtocolMarketDataSource.hpp"
#include "FixProtocolClient.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::FixProtocol;

namespace fix = trdk::Interaction::FixProtocol;
namespace gr = boost::gregorian;

fix::MarketDataSource::MarketDataSource(size_t index,
                                        Context &context,
                                        const std::string &instanceName,
                                        const IniSectionRef &conf)
    : Base(index, context, instanceName), m_settings(conf) {
  m_settings.Log(GetLog());
  m_settings.Validate();
}

void fix::MarketDataSource::Connect(const IniSectionRef &) {}

void fix::MarketDataSource::SubscribeToSecurities() {}

void fix::MarketDataSource::ResubscribeToSecurities() {}
