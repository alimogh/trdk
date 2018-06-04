//
//    Created: 2018/05/26 2:06 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "Config.hpp"
#include "Lib/SourcePropertiesDialog.hpp"
#include "Lib/SourcesListWidget.hpp"

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;
namespace fs = boost::filesystem;
namespace ptr = boost::property_tree;

namespace {

void CreateConfigFile(const fs::path &path) {
  ptr::ptree config;

  config.add("general.isReplayMode", false);
  config.add("general.tradingLog.isEnabled", true);
  config.add("general.marketDataLog.isEnabled", false);

  config.add("defaults.currency", "BTC");
  config.add("defaults.securityType", "CRYPTO");
  {
    const std::string symbols[] = {"BTC_EUR", "BTC_USD",  "ETH_BTC",
                                   "ETH_EUR", "ETH_USD",  "DOGE_BTC",
                                   "LTC_BTC", "DASH_BTC", "BCH_BTC"};
    ptr::ptree symbolsConfig;
    for (const auto &symbol : symbols) {
      symbolsConfig.push_back({"", ptr::ptree().put("", symbol)});
    }
    config.add_child("defaults.symbols", symbolsConfig);
  }

  config.add("riskControl.isEnabled", false);

  try {
    fs::create_directories(path.branch_path());
  } catch (const fs::filesystem_error &ex) {
    boost::format error("Failed to create default configuration file");
    error % ex.what();
    throw Exception(error.str().c_str());
  }

  std::ofstream file(path.string().c_str());
  if (!file) {
    throw Exception("Failed to create default configuration file");
  }
  ptr::json_parser::write_json(file, config, true);
}
}  // namespace

void FrontEnd::CheckConfig(const fs::path &configFilePath) {
  if (!fs::exists(configFilePath)) {
    CreateConfigFile(configFilePath);
  }

  ptr::ptree config;
  {
    std::ifstream file(configFilePath.string().c_str());
    ptr::json_parser::read_json(file, config);
    {
      const auto &tradingSystems = config.get_child_optional("sources");
      if (tradingSystems && !tradingSystems->empty()) {
        return;
      }
    }
  }

  if (QMessageBox::question(
          nullptr, QObject::tr("Configuration required"),
          QObject::tr("The configuration does not have access to any "
                      "trading system. It is required to configure access "
                      "to one or more trading systems.\n\nSetup trading "
                      "systems?"),
          QMessageBox::No | QMessageBox::Yes) != QMessageBox::Yes) {
    throw CheckConfigException(
        "The configuration does not have access to any trading system");
  }

  QDialog sourceListDialog;
  auto &listWidget = *new SourcesListWidget({}, &sourceListDialog);
  {
    SourcePropertiesDialog firstSourceDialog(
        boost::none, listWidget.GetImplementations(), nullptr);
    if (firstSourceDialog.exec() != QDialog::Accepted) {
      throw CheckConfigException(
          "The configuration does not have access to any trading system");
    }
    listWidget.AddSource(firstSourceDialog.Dump());
  }

  {
    sourceListDialog.setWindowTitle(QObject::tr("Source List"));
    auto &vLayout = *new QVBoxLayout(&sourceListDialog);
    sourceListDialog.setLayout(&vLayout);
    vLayout.addWidget(&listWidget);
    {
      auto &addButton =
          *new QPushButton(QObject::tr("Add New..."), &sourceListDialog);
      vLayout.addWidget(&addButton);
      Verify(QObject::connect(&addButton, &QPushButton::clicked,
                              &sourceListDialog, [&listWidget]() {
                                SourcePropertiesDialog dialog(
                                    boost::none,
                                    listWidget.GetImplementations(), nullptr);
                                if (dialog.exec() != QDialog::Accepted) {
                                  return;
                                }
                                listWidget.AddSource(dialog.Dump());
                              }));
    }
    {
      auto &okButton = *new QPushButton(QObject::tr("OK"), &sourceListDialog);
      auto &cancelButton =
          *new QPushButton(QObject::tr("Cancel"), &sourceListDialog);
      Verify(QObject::connect(&okButton, &QPushButton::clicked,
                              &sourceListDialog, &QDialog::accept));
      Verify(QObject::connect(&cancelButton, &QPushButton::clicked,
                              &sourceListDialog, &QDialog::reject));
      auto &hLayout = *new QHBoxLayout(&sourceListDialog);
      hLayout.addItem(
          new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));
      hLayout.addWidget(&okButton);
      hLayout.addWidget(&cancelButton);
      vLayout.addLayout(&hLayout);
    }
  }

  if (sourceListDialog.exec() != QDialog::Accepted ||
      listWidget.GetSize() == 0) {
    throw CheckConfigException(
        "The configuration does not have access to any trading system");
  }

  config.put_child("sources", listWidget.Dump());

  {
    std::ofstream file(configFilePath.string().c_str());
    ptr::json_parser::write_json(file, config, true);
  }
}
