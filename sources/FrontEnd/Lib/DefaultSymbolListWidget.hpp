//
//    Created: 2018/11/22 11:02
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

class TRDK_FRONTEND_LIB_API DefaultSymbolListWidget : public QWidget {
  Q_OBJECT

 public:
  typedef QWidget Base;

  explicit DefaultSymbolListWidget(const boost::property_tree::ptree &,
                                   QWidget *parent);
  ~DefaultSymbolListWidget();

 public slots:
  void RemoveSelected();

 public:
  void Add(std::string);

  size_t GetSize() const;

  boost::property_tree::ptree Dump() const;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace FrontEnd
}  // namespace trdk