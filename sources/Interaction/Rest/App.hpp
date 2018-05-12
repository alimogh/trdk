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
namespace Rest {

class TRDK_INTERACTION_REST_API App {
  App();
  ~App();

 public:
  App(const App &) = delete;
  App(App &&) = delete;

  static App &GetInstance();
};
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
