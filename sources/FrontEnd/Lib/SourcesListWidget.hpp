//
//    Created: 2018/05/26 1:47 PM
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

class TRDK_FRONTEND_LIB_API SourcesListWidget : public QWidget {
  Q_OBJECT

 public:
  typedef QWidget Base;

  explicit SourcesListWidget(const boost::property_tree::ptree &,
                             QWidget *parent);
  ~SourcesListWidget();

  boost::unordered_set<std::string> GetImplementations() const;

  void AddSource(const boost::property_tree::ptree &);

  size_t GetSize() const;

  const boost::property_tree::ptree &Dump() const;
  const boost::property_tree::ptree &Dump(const QModelIndex &) const;

 signals:
  void SourceDoubleClicked(const QModelIndex &);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace FrontEnd
}  // namespace trdk