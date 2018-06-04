/*******************************************************************************
 *   Created: 2017/09/10 00:41:14
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Api.h"
#include "Fwd.hpp"

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API Engine : public QObject {
  Q_OBJECT

 public:
  explicit Engine(const boost::filesystem::path& configFile,
                  const boost::filesystem::path& logsDir,
                  QWidget* parent);
  ~Engine();

 signals:
  void StateChange(bool isStarted);
  void Message(const QString&, bool isCritical);
  void LogRecord(const QString&);

  void OperationUpdate(const Orm::Operation&);
  void OrderUpdate(const Orm::Order&);

  void PriceUpdate(const Security*);

  void BarUpdate(const Security*, const Bar&);

 public:
  bool IsStarted() const;

  Context& GetContext();
  const Context& GetContext() const;

  const DropCopy& GetDropCopy() const;

  RiskControlScope& GetRiskControl(const TradingMode&);

  std::vector<boost::shared_ptr<Orm::Operation>> GetOperations(
      const QDateTime& startTime,
      const boost::optional<QDateTime>& endTime,
      bool isTradesIncluded = true,
      bool isErrorsIncluded = true,
      bool isCancelsIncluded = true,
      const boost::optional<QString>& strategy = boost::none) const;

  boost::property_tree::ptree LoadConfig() const;
  void StoreConfig(const boost::property_tree::ptree&);

  void StoreConfig(const Strategy&,
                   const boost::property_tree::ptree& config,
                   bool isActive);

  void ForEachActiveStrategy(
      const boost::function<void(const QUuid& typeIt,
                                 const QUuid& instanceId,
                                 const QString& name,
                                 const boost::property_tree::ptree& config)>&)
      const;

  std::vector<QString> GetStrategyNameList() const;

  std::string GenerateNewStrategyInstanceName(
      const std::string& nameBase) const;

  void Start(const boost::function<void(const std::string&)>& progressCallback);
  void Stop();

#ifdef DEV_VER
  void Test();
#endif

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace FrontEnd
}  // namespace trdk
