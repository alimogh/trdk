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

namespace {

class IniFileWizard : public IniFile {
 public:
  explicit IniFileWizard(const fs::path &path) : IniFile(FindIniFile(path)) {}

 private:
  static fs::path FindIniFile(const fs::path &source) {
    if (fs::exists(source)) {
      return source;
    }
    try {
      fs::create_directories(source.branch_path());
    } catch (const fs::filesystem_error &ex) {
      boost::format error("Failed to create default configuration file");
      error % ex.what();
      throw Error(error.str().c_str());
    }
    {
      std::ofstream file(source.string().c_str());
      if (!file) {
        throw Error("Failed to create default configuration file");
      }

      file << "[General]" << std::endl;
      file << "\tis_replay_mode = no" << std::endl;
      file << std::endl;
      file << "\ttrading_log = yes" << std::endl;
      file << "\tmarket_data_log = no" << std::endl;
      file << std::endl;
      file << "\t; When switch contract to the next. Zero - at the first"
           << std::endl;
      file << "\t; minutes of the expiration day. One - one day before"
           << std::endl;
      file << "\t; of the expiration day." << std::endl;
      file << "\tnumber_of_days_before_expiry_day_to_switch_contract = 0"
           << std::endl;
      file << std::endl;

      file << "[Defaults]" << std::endl;
      file << "\tcurrency = BTC" << std::endl;
      file << "\tsecurity_type = CRYPTO" << std::endl;
      file << "\tsymbol_list = BTC_EUR, BTC_USD, ETH_BTC, ETH_EUR, ETH_USD, "
              "DOGE_BTC, LTC_BTC, DASH_BTC, BCH_BTC"
           << std::endl;

      file << "[RiskControl]" << std::endl;
      file << "\tis_enabled = false" << std::endl;
      file << std::endl;
    }
    return source;
  }
};

}  // namespace

void FrontEnd::CheckConfig(const fs::path &configFilePath) {
  {
    for (const auto &section :
         IniFileWizard(configFilePath).ReadSectionsList()) {
      if (boost::istarts_with(section, "TradingSystemAndMarketDataSource.")) {
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
  auto &listWidget = *new SourcesListWidget(&sourceListDialog);
  {
    SourcePropertiesDialog firstSourceDialog(listWidget.GetTags(), nullptr);
    if (firstSourceDialog.exec() != QDialog::Accepted) {
      throw CheckConfigException(
          "The configuration does not have access to any trading system");
    }
    listWidget.AddSource(firstSourceDialog.GetSerializedProperties());
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
      Verify(QObject::connect(
          &addButton, &QPushButton::clicked, &sourceListDialog,
          [&listWidget]() {
            SourcePropertiesDialog dialog(listWidget.GetTags(), nullptr);
            if (dialog.exec() != QDialog::Accepted) {
              return;
            }
            listWidget.AddSource(dialog.GetSerializedProperties());
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

  {
    std::ofstream file(configFilePath.string().c_str(), std::ofstream::out |
                                                            std::ofstream::app |
                                                            std::ofstream::ate);
    listWidget.DumpList(file);
  }
}
