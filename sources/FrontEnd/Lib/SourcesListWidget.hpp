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

  explicit SourcesListWidget(QWidget *parent);
  ~SourcesListWidget();

  boost::unordered_set<std::string> GetTags() const;

  void AddSource(const boost::property_tree::ptree &);

  size_t GetSize() const;

  void DumpList(std::ostream &) const;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace FrontEnd
}  // namespace trdk