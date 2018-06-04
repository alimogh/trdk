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
      const boost::optional<ptr::ptree> &config,
      const boost::unordered_set<std::string> &disabledImplementations,
      SourcePropertiesDialog &self)
      : m_self(self) {
    m_ui.setupUi(&m_self);

    {
      std::string currentImpl;
      if (config) {
        currentImpl = config->get<std::string>("impl");
      }
      const auto &addExchange = [&currentImpl, &disabledImplementations, this](
                                    const char *title, const char *impl) {
        if ((!currentImpl.empty() && currentImpl != impl) ||
            disabledImplementations.count(impl)) {
          return;
        }
        m_ui.exchange->addItem(title, impl);
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

    if (config) {
      m_ui.exchange->setEnabled(false);
      m_ui.apiKey->setText(QString::fromStdString(
          config->get<std::string>("config.auth.apiKey")));
      m_ui.apiSecret->setText(QString::fromStdString(
          config->get<std::string>("config.auth.apiSecret")));
      if (m_ui.apiPassphrase->isEnabled()) {
        m_ui.apiPassphrase->setText(QString::fromStdString(
            config->get<std::string>("config.auth.apiPassphrase")));
      }
    }

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

  ptr::ptree Dump() const {
    const auto &impl = m_ui.exchange->currentData().toString().toStdString();
    ptr::ptree result;
    result.add("impl", impl);
    result.add("tradingMode", "live");
    result.add("title", m_ui.exchange->currentText().toStdString());
    if (impl != "Exmo") {
      result.add("module", "Rest");
      result.add("factory", "Create" + impl);
    } else {
      result.add("module", impl);
    }
    result.add("config.auth.apiKey", m_ui.apiKey->text().toStdString());
    result.add("config.auth.apiSecret", m_ui.apiSecret->text().toStdString());
    if (m_ui.apiPassphrase->isEnabled()) {
      result.add("config.auth.apiPassphrase",
                 m_ui.apiPassphrase->text().toStdString());
    }
    return result;
  }
};

SourcePropertiesDialog::SourcePropertiesDialog(
    const boost::optional<ptr::ptree> &config,
    const boost::unordered_set<std::string> &disabledImplementations,
    QWidget *parent)
    : Base(parent),
      m_pimpl(boost::make_unique<Implementation>(
          config, disabledImplementations, *this)) {}
SourcePropertiesDialog::~SourcePropertiesDialog() = default;

ptr::ptree SourcePropertiesDialog::Dump() const { return m_pimpl->Dump(); }
