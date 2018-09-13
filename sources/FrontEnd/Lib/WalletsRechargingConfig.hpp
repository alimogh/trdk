//
//    Created: 2018/09/12 11:00 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API WalletsRechargingConfig {
 public:
  WalletsRechargingConfig();
  WalletsRechargingConfig(WalletsRechargingConfig &&) noexcept;
  WalletsRechargingConfig(const WalletsRechargingConfig &) = delete;
  WalletsRechargingConfig &operator=(WalletsRechargingConfig &&) = delete;
  WalletsRechargingConfig &operator=(const WalletsRechargingConfig &) = delete;
  ~WalletsRechargingConfig();

  const QMap<QString, boost::unordered_set<const TradingSystem *>> &GetSources()
      const;
  QMap<QString, boost::unordered_set<const TradingSystem *>> &GetSources();

  void Load();
  void Save();

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace FrontEnd
}  // namespace trdk
