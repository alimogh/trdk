//
//    Created: 2018/06/14 19:19
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "SecurityListView.hpp"
#include "ItemDelegate.hpp"

using namespace trdk::FrontEnd;

class SecurityListView::Implementation {
 public:
  SecurityListView &m_self;

  void InitContextMenu() {
    m_self.setContextMenuPolicy(Qt::ActionsContextMenu);
    {
      auto &action = *new QAction(m_self.tr("Chart"), &m_self);
      Verify(m_self.connect(&action, &QAction::triggered, [this]() {
        ForEachSelectedSecurity([this](Security &security) {
          emit m_self.ChartRequested(
              QString::fromStdString(security.GetSymbol().GetSymbol()));
        });
      }));
      m_self.addAction(&action);
    }
    {
      auto &separator = *new QAction(&m_self);
      separator.setSeparator(true);
      m_self.addAction(&separator);
    }
    {
      auto &action = *new QAction(m_self.tr("Copy\tCtrl+C"), &m_self);
      action.setShortcut(QKeySequence::Copy);
      action.setShortcutContext(Qt::WidgetWithChildrenShortcut);
      Verify(m_self.connect(&action, &QAction::triggered,
                            [this]() { CopySelectedValuesToClipboard(); }));
      m_self.addAction(&action);
    }
  }

  void CopySelectedValuesToClipboard() const {
    auto *selection = m_self.selectionModel();
    const auto &indexes = selection->selectedIndexes();
    const QModelIndex *previous = nullptr;
    QString result;
    for (const auto &current : indexes) {
      if (previous) {
        current.parent().row() != previous->parent().row()
            ? result.append('\n')
            : result.append(", ");
      }
      result.append(m_self.model()->data(current).toString());
      previous = &current;
    }
    QApplication::clipboard()->setText(result);
  }

  template <typename Callback>
  void ForEachSelectedSecurity(const Callback &callback) {
    for (auto &index : m_self.selectionModel()->selectedIndexes()) {
      callback(ResolveModelIndexItem<Security>(index));
    }
  }
};

SecurityListView::SecurityListView(QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>({*this})) {
  setWindowTitle(tr("Securities"));
  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);
  setAlternatingRowColors(true);
  setSelectionBehavior(SelectItems);
  setSelectionMode(ExtendedSelection);
  verticalHeader()->setVisible(false);
  setItemDelegate(new ItemDelegate(this));

  m_pimpl->InitContextMenu();
}
SecurityListView::~SecurityListView() = default;
