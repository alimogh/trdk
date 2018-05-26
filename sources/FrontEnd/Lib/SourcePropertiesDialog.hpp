//
//    Created: 2018/05/26 2:13 PM
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

class TRDK_FRONTEND_LIB_API SourcePropertiesDialog : public QDialog {
  Q_OBJECT
 public:
  typedef QDialog Base;

  explicit SourcePropertiesDialog(
      const boost::unordered_set<std::string> &disabledSources,
      QWidget *parent);
  SourcePropertiesDialog(SourcePropertiesDialog &&) = delete;
  SourcePropertiesDialog(const SourcePropertiesDialog &) = delete;
  SourcePropertiesDialog &operator=(SourcePropertiesDialog &&) = delete;
  SourcePropertiesDialog &operator=(const SourcePropertiesDialog &) = delete;
  ~SourcePropertiesDialog();

  boost::property_tree::ptree GetSerializedProperties() const;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace FrontEnd
}  // namespace trdk
