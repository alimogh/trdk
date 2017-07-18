/**************************************************************************
 *   Created: 2013/11/11 22:42:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "FuncTestList.hpp"

using namespace trdk;
using namespace trdk::Tests;

namespace {

struct CloseStopper : private boost::noncopyable {
  explicit CloseStopper(bool wait) : m_wait(wait) {}
  ~CloseStopper() {
    if (m_wait) {
      std::cout << "Press Enter to exit." << std::endl;
      getchar();
    }
  }

 private:
  const bool m_wait;
};
}

int main(int argc, char **argv) {
  std::unique_ptr<CloseStopper> closeStopper;
  std::string funcTest;
  for (int i = 1; (!closeStopper || funcTest.empty()) && i < argc; ++i) {
    const std::string arg(argv[i]);
    if (boost::iequals(arg, "wait")) {
      if (!closeStopper) {
        closeStopper = boost::make_unique<CloseStopper>(true);
      }
    } else if (funcTest.empty() && !boost::istarts_with(arg, "--gtest_")) {
      funcTest = std::move(arg);
    }
  }

  if (funcTest.empty()) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
  } else {
    return RunFuncTest(std::move(funcTest));
  }
}
