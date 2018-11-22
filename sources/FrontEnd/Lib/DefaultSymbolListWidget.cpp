//
//    Created: 2018/11/22 11:02
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "DefaultSymbolListWidget.hpp"
#include "GeneratedFiles/ui_DefaultSymbolListWidget.h"

using namespace trdk::FrontEnd;
namespace ptr = boost::property_tree;

class DefaultSymbolListWidget::Implementation {
 public:
  std::set<std::string> m_result;
  Ui::DefaultSymbolListWidget m_ui;

  void Sync() {
    const QSignalBlocker signalBlocker(m_ui.list);
    m_ui.list->clear();
    for (const auto &symbol : m_result) {
      const auto title = QString::fromStdString(symbol);
      auto &item = *new QListWidgetItem(title, m_ui.list);
      item.setData(Qt::UserRole, title);
      m_ui.list->addItem(&item);
    }
  }
};

DefaultSymbolListWidget::DefaultSymbolListWidget(const ptr::ptree &config,
                                                 QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>()) {
  for (const auto &node : config) {
    m_pimpl->m_result.emplace(node.second.get_value<std::string>());
  }
  m_pimpl->m_ui.setupUi(this);
  m_pimpl->Sync();
}
DefaultSymbolListWidget::~DefaultSymbolListWidget() = default;

void DefaultSymbolListWidget::Add(std::string symbol) {
  m_pimpl->m_result.insert(std::move(symbol));
  m_pimpl->Sync();
}

void DefaultSymbolListWidget::RemoveSelected() {
  const auto &item = m_pimpl->m_ui.list->currentItem();
  if (!item) {
    return;
  }
  m_pimpl->m_result.erase(item->data(Qt::UserRole).toString().toStdString());
  m_pimpl->Sync();
}

size_t DefaultSymbolListWidget::GetSize() const {
  return m_pimpl->m_result.size();
}

ptr::ptree DefaultSymbolListWidget::Dump() const {
  ptr::ptree result;
  for (const auto &symbol : m_pimpl->m_result) {
    result.push_back({"", ptr::ptree().put("", symbol)});
  }
  return result;
}
