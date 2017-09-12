/*******************************************************************************
 *   Created: 2017/09/24 12:47:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once
#include "ui_EngineWidget.h"

namespace trdk {
namespace FrontEnd {
namespace WilliamCarry {

class EngineWidget : public QWidget {
  Q_OBJECT

 public:
  explicit EngineWidget(QWidget *parent);
  virtual ~EngineWidget() override = default;

 private:
  void InsertSecurity(const QString &symbol);
  void ShowSecurityWindow(const QString &symbol);
  void CloseSecurityWindow(const QString &symbol);

 private:
  Ui::EngineWidget m_ui;
  boost::unordered_map<QString, std::unique_ptr<SecurityWindow>> m_securities;
};
}
}
}