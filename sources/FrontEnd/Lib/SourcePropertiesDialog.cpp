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
  Ui::SourcePropertiesDialog m_ui{};

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
        if (currentImpl.empty() ? disabledImplementations.count(impl)
                                : currentImpl != impl) {
          return;
        }
        m_ui.exchange->addItem(title, impl);
      };
      addExchange("Coinbase Pro", "Coinbase");
      addExchange("Kraken", "Kraken");
      addExchange("CEX.IO", "Cexio");
      addExchange("Binance", "Binance");
      addExchange("Bittrex", "Bittrex");
      addExchange("EXMO", "Exmo");
      addExchange("Huobi", "Huobi");
      addExchange("Poloniex", "Poloniex");
      addExchange("Livecoin", "Livecoin");
      addExchange("Cryptopia", "Cryptopia");
      addExchange("YoBit.Net", "Yobitnet");
      addExchange("C-CEX", "Ccex");
      addExchange("CREX24", "Crex24");
      addExchange("ExcambioRex", "Excambiorex");
    }
    CheckFieldList();

    if (config) {
      m_ui.exchange->setEnabled(false);
      m_ui.enabled->setChecked(config->get<bool>("isEnabled", true));
      if (m_ui.userId->isEnabled()) {
        const auto &path = m_ui.exchange->currentData().toString() == "Huobi"
                               ? "config.account"
                               : "config.auth.userId";
        m_ui.userId->setText(
            QString::fromStdString(config->get<std::string>(path, "")));
      }
      m_ui.apiKey->setText(QString::fromStdString(
          config->get<std::string>("config.auth.apiKey", "")));
      m_ui.apiSecret->setText(QString::fromStdString(
          config->get<std::string>("config.auth.apiSecret", "")));
      if (m_ui.apiPassphrase->isEnabled()) {
        m_ui.apiPassphrase->setText(QString::fromStdString(
            config->get<std::string>("config.auth.apiPassphrase", "")));
      }
    }

    Verify(connect(
        m_ui.exchange,
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        [this](int) { CheckFieldList(); }));
  }

  void CheckFieldList() {
    {
      const auto &exchange = m_ui.exchange->currentData().toString();
      const auto hasUserId = exchange == "Cexio" || exchange == "Huobi";
      m_ui.userId->setEnabled(hasUserId);
      m_ui.userId->setEnabled(hasUserId);
    }
    {
      const auto hasPassphrase =
          m_ui.exchange->currentData().toString() == "Coinbase";
      m_ui.apiPassphraseLabel->setEnabled(hasPassphrase);
      m_ui.apiPassphrase->setEnabled(hasPassphrase);
    }
  }

  ptr::ptree Dump() const {
    const auto &impl = m_ui.exchange->currentData().toString().toStdString();
    ptr::ptree result;
    result.add("impl", impl);
    result.add("tradingMode", "live");
    result.add("title", m_ui.exchange->currentText().toStdString());
    if (impl != "Exmo" && impl != "Huobi" && impl != "Kraken" &&
        impl != "Binance" && impl != "Coinbase" && impl != "Poloniex" &&
        impl != "Bittrex") {
      result.add("module", "Rest");
      result.add("factory", "Create" + impl);
    } else {
      result.add("module", impl);
    }
    result.add("isEnabled", m_ui.enabled->isChecked());
    if (m_ui.userId->isEnabled()) {
      if (impl == "Huobi") {
        result.add("config.account", m_ui.userId->text().toStdString());
      } else {
        result.add("config.auth.userId", m_ui.userId->text().toStdString());
      }
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
