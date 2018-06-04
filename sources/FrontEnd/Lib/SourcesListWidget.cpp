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
  ptr::ptree m_result;
  Ui::SourcesListWidget m_ui;

  void Sync() {
    const QSignalBlocker signalBlocker(m_ui.list);
    m_ui.list->clear();
    for (const auto &node : m_result) {
      auto &item = *new QListWidgetItem(
          QString::fromStdString(node.second.get<std::string>("title")),
          m_ui.list);
      item.setData(Qt::UserRole, QString::fromStdString(node.first));
      m_ui.list->addItem(&item);
    }
  }
};

SourcesListWidget::SourcesListWidget(const ptr::ptree &config, QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>({config})) {
  m_pimpl->m_ui.setupUi(this);
  m_pimpl->Sync();
  Verify(QObject::connect(
      m_pimpl->m_ui.list, &QListWidget::doubleClicked,
      [this](const QModelIndex &index) { emit SourceDoubleClicked(index); }));
}
SourcesListWidget::~SourcesListWidget() = default;

boost::unordered_set<std::string> SourcesListWidget::GetImplementations()
    const {
  boost::unordered_set<std::string> result;
  for (const auto &node : m_pimpl->m_result) {
    Verify(result.emplace(node.second.get<std::string>("impl")).second);
  }
  return result;
}

void SourcesListWidget::AddSource(const ptr::ptree &config) {
  {
    auto tag = config.get<std::string>("impl");
    if (!tag.empty()) {
      tag[0] = static_cast<char>(std::tolower(tag[0]));
    }
    m_pimpl->m_result.put_child(tag, config);
  }
  m_pimpl->Sync();
}

size_t SourcesListWidget::GetSize() const { return m_pimpl->m_result.size(); }

const ptr::ptree &SourcesListWidget::Dump(const QModelIndex &index) const {
  return m_pimpl->m_result.get_child(
      index.data(Qt::UserRole).toString().toStdString());
}

const ptr::ptree &SourcesListWidget::Dump() const { return m_pimpl->m_result; }
