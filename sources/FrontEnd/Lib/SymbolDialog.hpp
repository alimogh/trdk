//
//    Created: 2018/06/05 12:52 AM
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

class TRDK_FRONTEND_LIB_API SymbolDialog : public QDialog {
  Q_OBJECT
 public:
  typedef QDialog Base;

  explicit SymbolDialog(QWidget *parent);
  SymbolDialog(SymbolDialog &&) = delete;
  SymbolDialog(const SymbolDialog &) = delete;
  SymbolDialog &operator=(SymbolDialog &&) = delete;
  SymbolDialog &operator=(const SymbolDialog &) = delete;
  ~SymbolDialog();

  QString GetSymbol() const;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace FrontEnd
}  // namespace trdk
