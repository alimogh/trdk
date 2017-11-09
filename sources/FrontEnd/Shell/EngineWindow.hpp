/*******************************************************************************
 *   Created: 2017/09/09 15:07:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Lib/Engine.hpp"
#include "ui_EngineWindow.h"

namespace trdk {
namespace FrontEnd {
namespace Shell {

class EngineWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit EngineWindow(const boost::filesystem::path &configsBase,
                        const boost::filesystem::path &configFileSubPath,
                        QWidget *parent);
  virtual ~EngineWindow() override = default;

 public:
  const QString &GetName() const { return m_name; }

 public:
  const boost::filesystem::path &GetConfigFilePath() const {
    return m_engine.GetConfigFilePath();
  }

 public slots:
  void ShowOrderWindow(Security &);
  void CloseOrderWindow(const trdk::Lib::Symbol &);

 private slots:
  void Start(bool start);
  void Stop(bool stop);

  void OnStateChanged(bool isStarted);
  void OnMessage(const QString &, bool isWarning);
  void OnLogRecord(const QString &);

  void OnOrder(const QString &id, int, double remainingQty);
  void OnTrade(const QString &orderId,
               int,
               double remainingQty,
               QString tradeId,
               double tradeQty,
               double tradePrice);

 private:
  void LoadModule();

 private:
  Lib::Engine m_engine;
  const QString m_name;
  Ui::EngineWindow m_ui;
  std::unique_ptr<trdk::Lib::Dll> m_moduleDll;
  std::vector<std::unique_ptr<QWidget>> m_modules;
  boost::unordered_map<trdk::Lib::Symbol, std::unique_ptr<OrderWindow>>
      m_orderWindows;
};
}
}
}
