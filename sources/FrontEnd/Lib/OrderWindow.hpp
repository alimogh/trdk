/*******************************************************************************
 *   Created: 2017/09/30 04:35:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API OrderWindow : public QMainWindow {
  Q_OBJECT

 public:
  typedef QMainWindow Base;

  explicit OrderWindow(Security &, Engine &, QWidget *parent);
  OrderWindow(OrderWindow &&) = delete;
  OrderWindow(const OrderWindow &) = delete;
  OrderWindow &operator=(OrderWindow &&) = delete;
  OrderWindow &operator=(const OrderWindow &) = delete;
  ~OrderWindow() override;

 signals:
  void Closed();

 protected:
  void closeEvent(QCloseEvent *) override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace FrontEnd
}  // namespace trdk