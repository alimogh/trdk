/*******************************************************************************
 *   Created: 2017/10/09 22:43:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Interaction {
namespace RestJson {

class App : private boost::noncopyable {
 private:
  App();
  ~App();

 public:
  static App &GetInstance();

 private:
};
}
}
}
