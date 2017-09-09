/*******************************************************************************
 *   Created: 2017/09/09 01:39:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

namespace trdk {
namespace Frontend {
namespace Shell {

class EngineListModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  explicit EngineListModel(const boost::filesystem::path &configPathFolderPath,
                           QObject *parent = Q_NULLPTR);
  virtual ~EngineListModel() override = default;

 public:
  virtual QVariant data(const QModelIndex &index, int role) const override;
  virtual QModelIndex index(int row,
                            int column,
                            const QModelIndex &parent) const override;
  virtual QModelIndex parent(const QModelIndex &index) const override;
  virtual int rowCount(const QModelIndex &parent) const override;
  virtual int columnCount(const QModelIndex &parent) const override;

 protected:
  void *ExportEngine(int row) const;
  const Engine &ImportEngine(const QModelIndex &) const;

 private:
  const boost::filesystem::path m_configPathFolderPath;
  std::vector<Engine> m_engines;
};
}
}
}
