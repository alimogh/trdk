/*******************************************************************************
 *   Created: 2017/09/09 01:56:04
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

class Engine : private boost::noncopyable {
 public:
  explicit Engine(const boost::filesystem::path &);
  Engine(Engine &&);

 public:
  const boost::filesystem::path &GetConfigurationFilePath() const;

 private:
  const boost::filesystem::path m_path;
};
}
}
}
