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

  void OperationUpdated(const boost::shared_ptr<const OperationRecord>&);
  void OrderUpdated(const boost::shared_ptr<const OrderRecord>&);

  void PriceUpdated(const Security*);

  void BarUpdated(const Security*, const Bar&);

 public:
  bool IsStarted() const;

  Context& GetContext();
  const Context& GetContext() const;

  void LogDebug(const QString&);
  void LogInfo(const QString&);
  void LogWarn(const QString&);
  void LogError(const QString&);

  const DropCopy& GetDropCopy() const;

  RiskControlScope& GetRiskControl(const TradingMode&);

  QString ResolveTradingSystemTitle(const QString& instanceName) const;

  std::vector<boost::shared_ptr<OperationRecord>> GetOperations(
      const QDateTime& startTime,
      const boost::optional<QDateTime>& endTime,
      bool isTradesIncluded = true,
      bool isErrorsIncluded = true,
      bool isCancelsIncluded = true,
      const boost::optional<QString>& strategy = boost::none) const;

  boost::property_tree::ptree LoadConfig() const;
  void StoreConfig(const boost::property_tree::ptree&);

  void StoreConfig(const Strategy&, boost::property_tree::ptree, bool isActive);

  void ForEachActiveStrategy(
      const boost::function<void(const QUuid& typeIt,
                                 const QUuid& instanceId,
                                 const QString& name,
                                 const boost::property_tree::ptree& config)>&)
      const;

  std::vector<QString> GetStrategyNameList() const;

  static std::string GenerateNewStrategyInstanceName(
      const std::string& nameBase);

  void Start(const boost::function<void(const std::string&)>& progressCallback);
  void Stop();

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace FrontEnd
}  // namespace trdk
