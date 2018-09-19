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

class TRDK_FRONTEND_LIB_API WalletsConfig {
 public:
  struct Source {
    Volume minDeposit;
  };
  struct Recharging {
    const TradingSystem *source;
    Volume minDepositToRecharge;
    Volume minRechargingTransactionVolume;
  };
  struct Wallet {
    boost::optional<QString> address;
    boost::optional<Recharging> recharging;
  };
  struct Symbol {
    boost::optional<Source> source;
    Wallet wallet;
  };
  typedef QMap<QString, boost::unordered_map<const TradingSystem *, Symbol>>
      Symbols;

  explicit WalletsConfig(const Engine &);
  explicit WalletsConfig(const Engine &, const boost::property_tree::ptree &);
  WalletsConfig(WalletsConfig &&) = default;
  WalletsConfig(const WalletsConfig &) = default;
  WalletsConfig &operator=(WalletsConfig &&) = default;
  WalletsConfig &operator=(const WalletsConfig &) = default;
  ~WalletsConfig() = default;

  const Symbols &Get() const;

  QString GetAddress(const QString &symbol, const TradingSystem &) const;

  void SetAddress(const QString &symbol,
                  const TradingSystem &,
                  const QString &);
  void RemoveAddress(const QString &symbol, const TradingSystem &);

  void SetRecharging(const QString &symbol,
                     const TradingSystem &,
                     const Recharging &);
  void RemoveRecharging(const QString &symbol, const TradingSystem &);

  void SetSource(const QString &symbol, const TradingSystem &, const Source &);
  void RemoveSource(const QString &symbol, const TradingSystem &);

  const boost::property_tree::ptree &Dump() const;

 private:
  Symbols Load() const;
  const TradingSystem *FindTradingSystem(const std::string &) const;

  const FrontEnd::Engine *m_engine;
  boost::property_tree::ptree m_config;
  boost::optional<Symbols> m_symbols;
};

}  // namespace FrontEnd
}  // namespace trdk
