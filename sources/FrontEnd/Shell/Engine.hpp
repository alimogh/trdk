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
namespace Frontend {
namespace Shell {

class Engine : private QObject {
  Q_OBJECT

 public:
  explicit Engine(const boost::filesystem::path &configFilePath,
                  QWidget *parent);
  ~Engine();

 public:
  const boost::filesystem::path &GetConfigFilePath() const;

 public:
  void Start();
  void Stop();

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
}