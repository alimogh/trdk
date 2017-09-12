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

#include "Engine.hpp"
#include "ui_EngineWindow.h"

namespace trdk {
namespace Frontend {
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

 private slots:
  void PinToTop();

  void Start();
  void Stop();

 private:
  Engine m_engine;
  const QString m_name;
  Ui::EngineWindow ui;
};
}
}
}
