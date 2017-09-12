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
namespace FrontEnd {
namespace Shell {

class EngineListModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  explicit EngineListModel(const boost::filesystem::path &configPathFolderPath,
                           QWidget *parent);
  virtual ~EngineListModel() override = default;

 public:
  EngineWindow &GetEngine(const QModelIndex &);
  const EngineWindow &GetEngine(const QModelIndex &) const;

 public:
  virtual QVariant data(const QModelIndex &index, int role) const override;
  virtual QModelIndex index(int row,
                            int column,
                            const QModelIndex &parent) const override;
  virtual QModelIndex parent(const QModelIndex &index) const override;
  virtual int rowCount(const QModelIndex &parent) const override;
  virtual int columnCount(const QModelIndex &parent) const override;

 private:
  std::vector<std::unique_ptr<EngineWindow>> m_engines;
};
}
}
}
