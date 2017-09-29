/*******************************************************************************
 *   Created: 2017/09/09 01:48:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "ShellEngineListModel.hpp"
#include "ShellEngineWindow.hpp"

using namespace trdk::Lib;
using namespace trdk::FrontEnd::Shell;

namespace fs = boost::filesystem;

EngineListModel::EngineListModel(const fs::path &configPathFolderPath,
                                 QWidget *parent)
    : QAbstractItemModel(parent) {
  foreach (
      const auto &it,
      std::make_pair(fs::recursive_directory_iterator(configPathFolderPath),
                     fs::recursive_directory_iterator())) {
    if (fs::is_regular_file(it)) {
      try {
        m_engines.emplace_back(boost::make_unique<EngineWindow>(
            configPathFolderPath, fs::relative(it.path(), configPathFolderPath),
            parent));
      } catch (const std::exception &ex) {
        qDebug() << QString("Failed to read engine configuration '%1': '%2'.")
                        .arg(QString::fromStdString(it.path().string()),
                             ex.what());
      }
    }
  }
}

EngineWindow &EngineListModel::GetEngine(const QModelIndex &index) {
  Assert(index.isValid());
  if (!index.isValid()) {
    throw LogicError("Engine index is not valid");
  }
  return *static_cast<EngineWindow *>(index.internalPointer());
}
const EngineWindow &EngineListModel::GetEngine(const QModelIndex &index) const {
  return const_cast<EngineListModel *>(this)->GetEngine(index);
}

QVariant EngineListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  switch (role) {
    case Qt::DisplayRole:
    case Qt::StatusTipRole:
      return GetEngine(index).GetName();
    case Qt::ToolTipRole:
      return QString::fromStdString(
          GetEngine(index).GetConfigFilePath().string());
  }

  return QVariant();
}

QModelIndex EngineListModel::index(int row,
                                   int column,
                                   const QModelIndex &parent) const {
  AssertEq(0, column);
  AssertLe(0, row);
  Assert(!parent.isValid());
  UseUnused(parent);

  if (column != 0 || row < 0 || row >= m_engines.size()) {
    return QModelIndex();
  }

  return createIndex(row, column,
                     &*const_cast<EngineListModel *>(this)->m_engines[row]);
}

QModelIndex EngineListModel::parent(const QModelIndex &) const {
  return QModelIndex();
}

int EngineListModel::rowCount(const QModelIndex &parent) const {
  Assert(!parent.isValid());
  UseUnused(parent);
  return static_cast<int>(m_engines.size());
}

int EngineListModel::columnCount(const QModelIndex &parent) const {
  Assert(!parent.isValid());
  UseUnused(parent);
  return 1;
}
