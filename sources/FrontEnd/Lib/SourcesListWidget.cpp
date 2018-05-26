//
//    Created: 2018/05/26 1:52 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "SourcesListWidget.hpp"
#include "GeneratedFiles/ui_SourcesListWidget.h"

using namespace trdk::FrontEnd;
namespace ptr = boost::property_tree;

class SourcesListWidget::Implementation {
 public:
  Ui::SourcesListWidget m_ui;
  boost::unordered_map<std::string, ptr::ptree> m_list;

  void Sync() {
    const QSignalBlocker signalBlocker(m_ui.list);
    m_ui.list->clear();
    for (const auto &node : m_list) {
      m_ui.list->addItem(
          QString::fromStdString(node.second.get<std::string>("title")));
    }
  }
};

SourcesListWidget::SourcesListWidget(QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>()) {
  m_pimpl->m_ui.setupUi(this);
}

SourcesListWidget::~SourcesListWidget() = default;

boost::unordered_set<std::string> SourcesListWidget::GetTags() const {
  boost::unordered_set<std::string> result;
  for (const auto &node : m_pimpl->m_list) {
    result.emplace(node.first);
  }
  return result;
}

void SourcesListWidget::AddSource(const ptr::ptree &node) {
  for (const auto &subNode : node) {
    m_pimpl->m_list[subNode.first] = subNode.second;
  }
  m_pimpl->Sync();
}

size_t SourcesListWidget::GetSize() const { return m_pimpl->m_list.size(); }

void SourcesListWidget::DumpList(std::ostream &os) const {
  for (const auto &node : m_pimpl->m_list) {
    os << "[TradingSystemAndMarketDataSource." << node.first << ']'
       << std::endl;
    for (const auto &subNode : node.second) {
      os << '\t' << subNode.first << " = "
         << subNode.second.get_value<std::string>() << std::endl;
    }
    os << std::endl;
  }
}
