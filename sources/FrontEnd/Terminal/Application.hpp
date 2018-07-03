//
//    Created: 2018/07/09 12:52
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

namespace trdk {
namespace FrontEnd {
namespace Terminal {

class Application : public QApplication {
  Q_OBJECT;

 public:
  typedef QApplication Base;

  explicit Application(int &argc, char **argv, int = ApplicationFlags);
  Application(Application &&) = delete;
  Application(const Application &) = delete;
  Application &operator=(Application &&) = delete;
  Application &operator=(const Application &) = delete;
  ~Application() override;

  void SetEngine(boost::shared_ptr<Engine>);

  bool notify(QObject *, QEvent *) override;

 private:
  boost::shared_ptr<Engine> m_engine;
};

}  // namespace Terminal
}  // namespace FrontEnd
}  // namespace trdk
