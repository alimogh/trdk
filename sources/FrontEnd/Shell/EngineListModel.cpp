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
#include "EngineListModel.hpp"
#include "Engine.hpp"

using namespace trdk::Lib;
using namespace trdk::Frontend::Shell;

namespace fs = boost::filesystem;

EngineListModel::EngineListModel(const fs::path &configPathFolderPath,
                                 QObject *parent)
    : QAbstractItemModel(parent), m_configPathFolderPath(configPathFolderPath) {
  foreach (
      const auto &it,
      std::make_pair(fs::recursive_directory_iterator(m_configPathFolderPath),
                     fs::recursive_directory_iterator())) {
    if (fs::is_regular_file(it)) {
      try {
        m_engines.emplace_back(it.path());
      } catch (const std::exception &ex) {
        qDebug() << QString("Failed to read engine configuration '%1': '%2'.")
                        .arg(QString::fromStdString(it.path().string()),
                             ex.what());
      }
    }
  }
}

void *EngineListModel::ExportEngine(int row) const {
  AssertLe(0, row);
  AssertLt(row, m_engines.size());
  return const_cast<Engine *>(&m_engines[row]);
}
const Engine &EngineListModel::ImportEngine(const QModelIndex &source) const {
  return *static_cast<const Engine *>(source.internalPointer());
}

QVariant EngineListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  switch (role) {
    case Qt::DisplayRole:
    case Qt::StatusTipRole: {
      QString result;
      const auto &engine = ImportEngine(index);
      auto path = engine.GetConfigurationFilePath().branch_path();
      std::vector<std::string> name;
      while (path != m_configPathFolderPath) {
        name.emplace_back(path.filename().string());
        path = path.branch_path();
      }
      {
        const auto &filename = engine.GetConfigurationFilePath().filename();
        if (boost::iequals(filename.extension().string(), ".ini")) {
          name.emplace_back(filename.stem().string());
        } else {
          name.emplace_back(filename.string());
        }
      }
      return QString::fromStdString(boost::join(name, " / "));
    }
    case Qt::ToolTipRole:
      return QString::fromStdString(
          ImportEngine(index).GetConfigurationFilePath().string());
    default:
      return QVariant();
  }
}

QModelIndex EngineListModel::index(int row,
                                   int column,
                                   const QModelIndex &parent) const {
  AssertEq(0, column);
  AssertLe(0, row);
  Assert(!parent.isValid());

  if (column != 0 || row < 0 || row >= m_engines.size()) {
    return QModelIndex();
  }

  return createIndex(row, column, ExportEngine(row));
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
