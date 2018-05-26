//
//    Created: 2018/05/26 2:13 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "SourcePropertiesDialog.hpp"
#include "GeneratedFiles/ui_SourcePropertiesDialog.h"

using namespace trdk;
using namespace FrontEnd;
namespace ptr = boost::property_tree;

class SourcePropertiesDialog::Implementation {
 public:
  SourcePropertiesDialog &m_self;
  Ui::SourcePropertiesDialog m_ui;

  explicit Implementation(
      const boost::unordered_set<std::string> &disabledSources,
      SourcePropertiesDialog &self)
      : m_self(self) {
    m_ui.setupUi(&m_self);

    {
      const auto &addExchange = [&disabledSources, this](const char *title,
                                                         const char *tag) {
        if (disabledSources.count(tag)) {
          return;
        }
        m_ui.exchange->addItem(title, tag);
      };
      addExchange("GDAX", "Gdax");
      addExchange("CEX.IO", "Cexio");
      addExchange("Bittrex", "Bittrex");
      addExchange("EXMO", "Exmo");
      addExchange("Livecoin", "Livecoin");
      addExchange("Cryptopia", "Cryptopia");
      addExchange("YoBit.Net", "Yobitnet");
      addExchange("C-CEX", "Ccex");
      addExchange("CREX24", "Crex24");
      addExchange("ExcambioRex", "Excambiorex");
    }
    CheckApiPassphrase();

    Verify(connect(
        m_ui.exchange,
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        [this](int) { CheckApiPassphrase(); }));
  }

  void CheckApiPassphrase() {
    const auto hasPassphrase =
        m_ui.exchange->currentData().toString() == "Gdax";
    m_ui.apiPassphraseLabel->setEnabled(hasPassphrase);
    m_ui.apiPassphrase->setEnabled(hasPassphrase);
  }

  ptr::ptree GetSerializedProperties() const {
    ptr::ptree nodes;
    nodes.put("title", m_ui.exchange->currentText().toStdString());
    const auto &tag = m_ui.exchange->currentData().toString().toStdString();
    if (tag != "Exmo") {
      nodes.put("module", "Rest");
      nodes.put("factory", tag);
    } else {
      nodes.put("module", tag);
    }
    nodes.put("api_key", m_ui.apiKey->text().toStdString());
    nodes.put("api_secret", m_ui.apiSecret->text().toStdString());
    if (m_ui.apiPassphrase->isEnabled()) {
      nodes.put("api_passphrase", m_ui.apiPassphrase->text().toStdString());
    }

    ptr::ptree result;
    result.add_child(tag, nodes);
    return result;
  }
};

SourcePropertiesDialog::SourcePropertiesDialog(
    const boost::unordered_set<std::string> &disabledSources, QWidget *parent)
    : Base(parent),
      m_pimpl(boost::make_unique<Implementation>(disabledSources, *this)) {}
SourcePropertiesDialog::~SourcePropertiesDialog() = default;

ptr::ptree SourcePropertiesDialog::GetSerializedProperties() const {
  return m_pimpl->GetSerializedProperties();
}
