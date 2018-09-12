//
//    Created: 2018/09/13 10:57
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "WalletsRechargingConfig.hpp"

using namespace trdk;
using namespace FrontEnd;

class WalletsRechargingConfig::Implementation {
 public:
  QMap<QString, boost::unordered_set<const TradingSystem *>> m_sources;
};

WalletsRechargingConfig::WalletsRechargingConfig()
    : m_pimpl(boost::make_unique<Implementation>()) {}
WalletsRechargingConfig::WalletsRechargingConfig(
    WalletsRechargingConfig &&) noexcept = default;
WalletsRechargingConfig::~WalletsRechargingConfig() = default;

const QMap<QString, boost::unordered_set<const TradingSystem *>>
    &WalletsRechargingConfig::GetSources() const {
  return m_pimpl->m_sources;
}
QMap<QString, boost::unordered_set<const TradingSystem *>>
    &WalletsRechargingConfig::GetSources() {
  return m_pimpl->m_sources;
}

void WalletsRechargingConfig::Load() { m_pimpl->m_sources.clear(); }

void WalletsRechargingConfig::Save() {}
