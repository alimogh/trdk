//
//    Created: 2018/07/09 12:56
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "Application.hpp"
#include "Lib/Engine.hpp"

using namespace trdk::FrontEnd;
using namespace Terminal;

Application::Application(int &argc, char **argv, const int flags)
    : Base(argc, argv, flags) {}
Application::~Application() = default;

void Application::SetEngine(boost::shared_ptr<Engine> engine) {
  Assert(!m_engine);
  m_engine = std::move(engine);
}

bool Application::notify(QObject *object, QEvent *event) {
  try {
    return Base::notify(object, event);
  } catch (const std::exception &ex) {
    if (m_engine) {
      m_engine->GetContext().GetLog().Error(
          R"(Unhandled terminal exception: "%1%".)", ex.what());
    }
    AssertFailNoException();
    throw;
  } catch (...) {
    if (m_engine) {
      m_engine->GetContext().GetLog().Error(
          "Unhandled terminal unknown exception.");
    }
    AssertFailNoException();
    throw;
  }
}