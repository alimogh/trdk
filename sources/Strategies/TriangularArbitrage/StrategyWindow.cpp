/*******************************************************************************
 *   Created: 2018/03/06 12:40:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "StrategyWindow.hpp"
#include "Strategy.hpp"

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;
using namespace Strategies::TriangularArbitrage;
namespace ta = Strategies::TriangularArbitrage;
namespace ptr = boost::property_tree;
namespace ids = boost::uuids;

namespace {

LegsConf ParseLegsConf(const ptr::ptree &config) {
  LegsConf result = {};
  auto it = result.begin();
  for (const auto &node : config) {
    const auto &legConf = node.second.get_value<std::string>();
    if (it >= result.cend() || legConf.size() < 2 ||
        (legConf[0] != '-' && legConf[0] != '+')) {
      throw Exception("Wrong leg configuration in leg set configuration");
    }
    it->side = legConf[0] == '+' ? ORDER_SIDE_LONG : ORDER_SIDE_SHORT;
    it->symbol = QString::fromStdString(legConf.substr(1));
    ++it;
  }
  if (it != result.cend()) {
    throw Exception("Wrong leg configuration in leg set configuration");
  }
  return result;
}

}  // namespace

StrategyWidgetList RestoreStrategyWidgets(Engine &engine,
                                          const QUuid &typeId,
                                          const QUuid &instanceId,
                                          const QString &name,
                                          const ptr::ptree &config,
                                          QWidget *parent) {
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  StrategyWidgetList result;
  if (ConvertToBoostUuid(typeId) == ta::Strategy::typeId) {
    result.emplace_back(boost::make_unique<StrategyWindow>(
        engine, instanceId, name, config, parent));
  }
  return result;
}

StrategyWidgetList CreateStrategyWidgetsForSymbols(Engine &engine,
                                                   const QString &configString,
                                                   QWidget *parent) {
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  StrategyWidgetList result;
  std::istringstream configStream(configString.toStdString());
  ptr::ptree config;
  ptr::json_parser::read_json(configStream, config);
  result.emplace_back(boost::make_unique<StrategyWindow>(
      engine, ParseLegsConf(config), parent));
  return result;
}

StrategyWindow::StrategyWindow(Engine &engine,
                               const LegsConf &legSet,
                               QWidget *parent)
    : Base(parent), m_engine(engine) {
  std::string legsStr;
  ptr::ptree legs;
  ptr::ptree symbols;
  {
    auto isFirst = true;
    for (const auto &leg : legSet) {
      if (!isFirst) {
        legsStr += ", ";
      } else {
        isFirst = false;
      }
      const auto &legStr =
          (leg.side == ORDER_SIDE_BUY ? '+' : '-') + leg.symbol.toStdString();
      legsStr += legStr;
      legs.push_back({"", ptr::ptree().put("", legStr)});
      symbols.push_back({"", ptr::ptree().put("", leg.symbol)});
    }
  }

  static boost::uuids::random_generator generateStrategyId;
  const auto &id = generateStrategyId();

  m_config.add("module", "TriangularArbitrage");
  m_config.add("id", id);
  m_config.add("isEnabled", true);
  m_config.add("tradingMode", "live");
  m_config.add("config.isTradingEnabled", false);
  m_config.add("config.minVolume", 0);
  m_config.add("config.maxVolume", 1000);
  m_config.add("config.minProfitRatio", 1.01);
  m_config.add_child("config.legs", legs);
  m_config.add_child("requirements.level1Updates.symbols", symbols);

  Init(id, m_engine.GenerateNewStrategyInstanceName("Triangular Arbitrage " +
                                                    legsStr));

  StoreConfig(true);
}

StrategyWindow::StrategyWindow(Engine &engine,
                               const QUuid &strategyId,
                               const QString &name,
                               const ptr::ptree &config,
                               QWidget *parent)
    : Base(parent), m_engine(engine), m_config(config) {
  Init(ConvertToBoostUuid(strategyId), name.toStdString());
}

StrategyWindow::~StrategyWindow() { m_strategy->Stop(); }

void StrategyWindow::Init(const ids::uuid &id,
                          const std::string &strategyName) {
  const auto &legSet = ParseLegsConf(m_config.get_child("config.legs"));

  m_investCurrency =
      legSet.front().symbol.right(legSet.front().symbol.indexOf('_'));
  m_resultCurrency =
      legSet.front().symbol.left(legSet.front().symbol.indexOf('_'));

  m_strategy = &CreateStrategyInstance(id, strategyName);

  m_ui.setupUi(this);
  m_legs = {
      Leg{m_ui.leg1Symbol, m_ui.leg1BestPrice, m_ui.leg1BestExchange,
          m_ui.leg1Frame},
      Leg{m_ui.leg2Symbol, m_ui.leg2BestPrice, m_ui.leg2BestExchange,
          m_ui.leg2Frame},
      Leg{m_ui.leg3Symbol, m_ui.leg3BestPrice, m_ui.leg3BestExchange,
          m_ui.leg3Frame},
  };

  {
    QStringList symbolList;
    for (const auto &leg : legSet) {
      symbolList << QString("%1%2").arg(leg.side == ORDER_SIDE_BUY ? "+" : "-",
                                        leg.symbol);
    }
    setWindowTitle(symbolList.join('*') + " " + tr("Triangular Arbitrage") +
                   " - " + QCoreApplication::applicationName());
  }

  m_ui.isActivated->setChecked(m_strategy->IsTradingEnabled());

  {
    m_strategy->GetContext().ForEachMarketDataSource(
        [&](const MarketDataSource &source) {
          const auto name = QString::fromStdString(source.GetTitle());
          const auto &addItem = [&](QListWidget &control) {
            auto &item = *new QListWidgetItem(name);
            control.addItem(&item);
            item.setData(Qt::UserRole, static_cast<int>(source.GetIndex()));
          };
          addItem(*m_ui.startExchanges);
          addItem(*m_ui.middleExchanges);
          addItem(*m_ui.finishExchanges);
        });
    SetSelectedStartExchanges();
    SetSelectedMiddleExchanges();
    SetSelectedFinishExchanges();
  }

  m_ui.minVolume->setValue(m_strategy->GetMinVolume());
  m_ui.maxVolume->setValue(m_strategy->GetMaxVolume());
  m_ui.volumeSymbol->setText(tr("Volume (%1):").arg(m_investCurrency));

  m_ui.minProfit->setValue((m_strategy->GetMinProfitRatio() - 1) * 100);

  {
    m_ui.opportunities->setColumnCount((static_cast<int>(legSet.size()) * 3) +
                                       5);
    {
      auto font = m_ui.opportunities->font();
      font.setBold(true);
      m_ui.opportunities->horizontalHeader()->setFont(font);
      m_ui.opportunities->verticalHeader()->setDefaultSectionSize(
          QFontMetrics(font).height() + 8);
    }

    auto column = 0;
    size_t legIndex = 0;
    AssertEq(m_legs.size(), legSet.size());
    for (const auto &leg : legSet) {
      const auto legName =
          QString("%1 %2").arg(ConvertToUiString(leg.side), leg.symbol);

      m_legs[legIndex].symbol->setText(legName);
      m_legs[legIndex].price->setStyleSheet(leg.side == ORDER_SIDE_SELL
                                                ? "color:rgb(230,59,1);"
                                                : "color:rgb(0,195,91);");
      m_legs[legIndex].frame->setStyleSheet(
          leg.side == ORDER_SIDE_SELL ? "border-color:rgb(230,59,1);"
                                      : "border-color:rgb(0,195,91);");

      m_ui.opportunities->setHorizontalHeaderItem(
          column++, new QTableWidgetItem(legName + " " + tr("Qty.")));
      m_ui.opportunities->setHorizontalHeaderItem(
          column++, new QTableWidgetItem(legName + " " + tr("Price")));
      m_ui.opportunities->setHorizontalHeaderItem(
          column++, new QTableWidgetItem(legName + " " + tr("Exchange")));
      ++legIndex;
    }
    m_ui.opportunities->setHorizontalHeaderItem(
        column++,
        new QTableWidgetItem(tr("Investment") + ", " + m_investCurrency));
    m_ui.opportunities->setHorizontalHeaderItem(
        column++, new QTableWidgetItem(tr("Reduces investment")));
    m_ui.opportunities->setHorizontalHeaderItem(
        column++, new QTableWidgetItem(tr("P&L") + ", %"));
    m_ui.opportunities->setHorizontalHeaderItem(
        column++, new QTableWidgetItem(tr("P&L") + ", " + m_resultCurrency));
    m_ui.opportunities->setHorizontalHeaderItem(
        column, new QTableWidgetItem(tr("Signaled")));
  }

  ConnectSignals();
}

void StrategyWindow::Disable() {
  m_ui.isActivated->setEnabled(false);
  m_ui.minVolume->setEnabled(false);
  m_ui.maxVolume->setEnabled(false);
  m_ui.minProfit->setEnabled(false);
  m_ui.startExchanges->setEnabled(false);
  m_ui.middleExchanges->setEnabled(false);
  m_ui.finishExchanges->setEnabled(false);
}

void StrategyWindow::Block(const QString &reason) {
  Disable();
  ShowBlockedStrategyMessage(reason, this);
}

void StrategyWindow::ConnectSignals() {
  qRegisterMetaType<std::vector<Opportunity>>("std::vector<Opportunity>");
  Verify(connect(this, &StrategyWindow::OpportunityUpdated, this,
                 &StrategyWindow::UpdateOpportunity, Qt::QueuedConnection));

  Verify(connect(this, &StrategyWindow::Blocked, this, &StrategyWindow::Block,
                 Qt::QueuedConnection));

  Verify(connect(
      m_ui.isActivated, &QCheckBox::toggled, [this](const bool isEnabled) {
        try {
          m_strategy->EnableTrading(isEnabled);
        } catch (const std::exception &ex) {
          QMessageBox::critical(this, QObject::tr("Failed to activate trading"),
                                ex.what(), QMessageBox::Ok);
        }
        {
          const QSignalBlocker blocker(*m_ui.isActivated);
          m_ui.isActivated->setChecked(m_strategy->IsTradingEnabled());
        }
        {
          m_config.put("config.isTradingEnabled",
                       m_strategy->IsTradingEnabled());
          StoreConfig(true);
        }
      }));

  Verify(connect(
      m_ui.startExchanges, &QListWidget::itemSelectionChanged, [this]() {
        try {
          m_strategy->SetStartExchanges(GetSelectedStartExchanges());
        } catch (const std::exception &ex) {
          QMessageBox::critical(this, QObject::tr("Failed to setup strategy"),
                                ex.what(), QMessageBox::Ok);
        }
        SetSelectedStartExchanges();
      }));
  Verify(connect(
      m_ui.middleExchanges, &QListWidget::itemSelectionChanged, [this]() {
        try {
          m_strategy->SetMiddleExchanges(GetSelectedMiddleExchanges());
        } catch (const std::exception &ex) {
          QMessageBox::critical(this, QObject::tr("Failed to setup strategy"),
                                ex.what(), QMessageBox::Ok);
        }
        SetSelectedMiddleExchanges();
      }));
  Verify(connect(
      m_ui.finishExchanges, &QListWidget::itemSelectionChanged, [this]() {
        try {
          m_strategy->SetFinishExchanges(GetSelectedFinishExchanges());
        } catch (const std::exception &ex) {
          QMessageBox::critical(this, QObject::tr("Failed to setup strategy"),
                                ex.what(), QMessageBox::Ok);
        }
        SetSelectedFinishExchanges();
      }));

  Verify(connect(
      m_ui.minVolume,
      static_cast<void (QDoubleSpinBox::*)(double)>(
          &QDoubleSpinBox::valueChanged),
      [this](const double value) {
        try {
          m_strategy->SetMinVolume(value);
        } catch (const std::exception &ex) {
          QMessageBox::critical(this, QObject::tr("Failed to setup strategy"),
                                ex.what(), QMessageBox::Ok);
        }
        {
          const QSignalBlocker blocker(*m_ui.minVolume);
          m_ui.minVolume->setValue(m_strategy->GetMinVolume());
        }
        {
          const QSignalBlocker blocker(*m_ui.maxVolume);
          m_ui.maxVolume->setValue(m_strategy->GetMaxVolume());
        }
        {
          m_config.put("config.minVolume", m_strategy->GetMinVolume());
          m_config.put("config.maxVolume", m_strategy->GetMaxVolume());
          StoreConfig(true);
        }
      }));
  Verify(connect(
      m_ui.maxVolume,
      static_cast<void (QDoubleSpinBox::*)(double)>(
          &QDoubleSpinBox::valueChanged),
      [this](const double value) {
        try {
          m_strategy->SetMaxVolume(value);
        } catch (const std::exception &ex) {
          QMessageBox::critical(this, QObject::tr("Failed to setup strategy"),
                                ex.what(), QMessageBox::Ok);
        }
        {
          const QSignalBlocker blocker(*m_ui.minVolume);
          m_ui.minVolume->setValue(m_strategy->GetMinVolume());
        }
        {
          const QSignalBlocker blocker(*m_ui.maxVolume);
          m_ui.maxVolume->setValue(m_strategy->GetMaxVolume());
        }
        {
          m_config.put("config.maxVolume", m_strategy->GetMaxVolume());
          StoreConfig(true);
        }
      }));

  Verify(connect(
      m_ui.minProfit,
      static_cast<void (QDoubleSpinBox::*)(double)>(
          &QDoubleSpinBox::valueChanged),
      [this](double value) {
        value = 1 + (value / 100);
        try {
          m_strategy->SetMinProfitRatio(value);
        } catch (const std::exception &ex) {
          QMessageBox::critical(this, QObject::tr("Failed to setup strategy"),
                                ex.what(), QMessageBox::Ok);
        }
        {
          const QSignalBlocker blocker(*m_ui.maxVolume);
          m_ui.minProfit->setValue((m_strategy->GetMinProfitRatio() - 1) * 100);
        }
        {
          m_config.put("config.minProfitRatio",
                       m_strategy->GetMinProfitRatio());
          StoreConfig(true);
        }
      }));
}

ta::Strategy &StrategyWindow::CreateStrategyInstance(const ids::uuid &id,
                                                     const std::string &name) {
  {
    ptr::ptree strategyTagedConfig;
    strategyTagedConfig.add_child(name, m_config);
    m_engine.GetContext().Add(strategyTagedConfig);
  }
  auto &result = *boost::polymorphic_downcast<ta::Strategy *>(
      &m_engine.GetContext().GetSrategy(id));

  m_opportunityUpdateConnection = result.SubscribeToOpportunity(
      [this](const std::vector<Opportunity> &opportunities) {
        emit OpportunityUpdated(opportunities);
      });
  m_blockConnection =
      result.SubscribeToBlocking([this](const std::string *reasonSource) {
        QString reason;
        if (reasonSource) {
          reason = QString::fromStdString(*reasonSource);
        }
        emit Blocked(reason);
      });

  return result;
}

void StrategyWindow::UpdateOpportunity(
    const std::vector<Opportunity> &opportunities) {
  if (!opportunities.empty()) {
    const auto &bestOpportunity = opportunities.front();
    {
      size_t i = 0;
      AssertEq(bestOpportunity.targets.size(), m_legs.size());
      for (const auto &target : bestOpportunity.targets) {
        m_legs[i].price->setText(ConvertPriceToText(target.price));
        m_legs[i].exchange->setText(
            QString::fromStdString(target.tradingSystem->GetTitle()));
        ++i;
      }
    }
    m_ui.pnlRatio->setText(
        bestOpportunity.pnlRatio.IsNotNan()
            ? QString::number((bestOpportunity.pnlRatio - 1) * 100, 'f', 2) +
                  '%'
            : "---");
    m_ui.pnlVolume->setText(
        bestOpportunity.checkError
            ? QString::fromStdString(*bestOpportunity.checkError)
            : bestOpportunity.pnlVolume.IsNotNan()
                  ? (ConvertVolumeToText(bestOpportunity.pnlVolume) + ' ' +
                     m_resultCurrency)
                  : QString());
    if (bestOpportunity.targets[LEG_1].qty.IsNotNan()) {
      m_ui.investmentVolume->setText(QString(
          tr("invt: %1 %2")
              .arg(ConvertVolumeToText(bestOpportunity.targets[LEG_1].qty *
                                       bestOpportunity.targets[LEG_1].price),
                   m_investCurrency)));
    } else {
      m_ui.investmentVolume->setText("");
    }

    static const QString signaledStyle("background-color:rgb(0,146,68);");
    static const QString errorStyle("background-color:rgb(230,59,1);");
    m_ui.operationResultFrame->setStyleSheet(
        bestOpportunity.isSignaled
            ? signaledStyle
            : bestOpportunity.checkError ? errorStyle : styleSheet());
  } else {
    for (const auto &leg : m_legs) {
      leg.price->setText(
          ConvertPriceToText(std::numeric_limits<double>::quiet_NaN()));
      leg.exchange->setText("---");
    }
    m_ui.pnlRatio->setText("---");
    m_ui.pnlVolume->setText("---");
    m_ui.investmentVolume->setText("");
  }

  {
    m_ui.opportunities->setRowCount(static_cast<int>(opportunities.size()));
    auto row = 0;
    for (const auto &opportunity : opportunities) {
      auto column = 0;
      for (const auto &target : opportunity.targets) {
        m_ui.opportunities->setItem(
            row, column++,
            new QTableWidgetItem(ConvertPriceToText(target.qty)));
        m_ui.opportunities->setItem(
            row, column++,
            new QTableWidgetItem(ConvertPriceToText(target.price)));
        m_ui.opportunities->setItem(row, column++,
                                    new QTableWidgetItem(QString::fromStdString(
                                        target.tradingSystem->GetTitle())));
      }
      m_ui.opportunities->setItem(
          row, column++,
          new QTableWidgetItem(
              opportunity.pnlRatio.IsNotNan()
                  ? ConvertVolumeToText(opportunity.targets[LEG_1].qty *
                                        opportunity.targets[LEG_1].price)
                  : QString()));
      m_ui.opportunities->setItem(
          row, column++,
          new QTableWidgetItem(
              opportunity.reducedByAccountBalanceLeg == numberOfLegs
                  ? QString()
                  : QString::fromStdString(
                        opportunity
                            .targets[opportunity.reducedByAccountBalanceLeg]
                            .tradingSystem->GetTitle())));
      m_ui.opportunities->setItem(
          row, column++,
          new QTableWidgetItem(
              opportunity.pnlRatio.IsNotNan()
                  ? QString::number((opportunity.pnlRatio - 1) * 100, 'f', 2)
                  : QString()));
      m_ui.opportunities->setItem(
          row, column++,
          new QTableWidgetItem(ConvertVolumeToText(opportunity.pnlVolume)));
      {
        auto signalState = opportunity.isSignaled ? tr("yes") : tr("no");
        if (opportunity.checkError) {
          signalState += ", ";
          if (opportunity.orderError) {
            std::ostringstream os;
            os << opportunity.errorTradingSystem->GetTitle() << " requires:";
            auto has = false;
            if (opportunity.orderError->qty) {
              os << " qty. >= " << *opportunity.orderError->qty;
              has = true;
            }
            if (opportunity.orderError->price) {
              if (has) {
                os << ',';
              }
              os << " price >= " << *opportunity.orderError->price;
              has = true;
            }
            if (opportunity.orderError->volume) {
              if (has) {
                os << ',';
              }
              os << " volume >= " << *opportunity.orderError->volume;
            }
            signalState += QString::fromStdString(os.str());
          } else {
            signalState += QString::fromStdString(*opportunity.checkError);
            if (opportunity.errorTradingSystem) {
              signalState += " (" +
                             QString::fromStdString(
                                 opportunity.errorTradingSystem->GetTitle()) +
                             ')';
            }
          }
        } else {
          Assert(!opportunity.errorTradingSystem);
        }
        m_ui.opportunities->setItem(row, column,
                                    new QTableWidgetItem(signalState));
      }
      ++row;
    }
    AssertEq(m_ui.opportunities->rowCount(), row);
    if (m_maxNumberOfOppotunities < m_ui.opportunities->rowCount()) {
      m_ui.opportunities->setVisible(false);
      std::vector<QString> headers;
      headers.reserve(m_ui.opportunities->columnCount());
      for (auto i = 0; i < m_ui.opportunities->columnCount(); ++i) {
        auto &header = *m_ui.opportunities->horizontalHeaderItem(i);
        headers.emplace_back(header.text());
        header.setText("");
      }
      m_ui.opportunities->resizeColumnsToContents();
      for (auto i = 0; i < m_ui.opportunities->columnCount(); ++i) {
        m_ui.opportunities->horizontalHeaderItem(i)->setText(headers[i]);
      }
      m_ui.opportunities->setVisible(true);
      m_maxNumberOfOppotunities = m_ui.opportunities->rowCount();
    }
  }
}

boost::unordered_set<size_t> StrategyWindow::GetSelectedStartExchanges() const {
  return GetSelectedExchanges(*m_ui.startExchanges,
                              m_strategy->GetStartExchanges());
}
boost::unordered_set<size_t> StrategyWindow::GetSelectedMiddleExchanges()
    const {
  return GetSelectedExchanges(*m_ui.middleExchanges,
                              m_strategy->GetMiddleExchanges());
}
boost::unordered_set<size_t> StrategyWindow::GetSelectedFinishExchanges()
    const {
  return GetSelectedExchanges(*m_ui.finishExchanges,
                              m_strategy->GetFinishExchanges());
}
boost::unordered_set<size_t> StrategyWindow::GetSelectedExchanges(
    const QListWidget &control,
    const boost::unordered_set<size_t> &defaultResult) {
  boost::unordered_set<size_t> result;
  const auto &selected = control.selectedItems();
  if (selected.size() == control.count()) {
    return result;
  }
  for (const auto &item : selected) {
    Verify(result.emplace(item->data(Qt::UserRole).toInt()).second);
  }
  return !result.empty() ? result : defaultResult;
}

void StrategyWindow::SetSelectedStartExchanges() {
  SetSelectedExchanges(*m_ui.startExchanges, m_strategy->GetStartExchanges());
}
void StrategyWindow::SetSelectedMiddleExchanges() {
  SetSelectedExchanges(*m_ui.middleExchanges, m_strategy->GetMiddleExchanges());
}
void StrategyWindow::SetSelectedFinishExchanges() {
  SetSelectedExchanges(*m_ui.finishExchanges, m_strategy->GetFinishExchanges());
}
void StrategyWindow::SetSelectedExchanges(
    QListWidget &control, const boost::unordered_set<size_t> &indexes) const {
  const QSignalBlocker blocker(control);

  if (indexes.empty()) {
    control.selectAll();
  } else {
    for (auto i = 0; i < control.count(); ++i) {
      auto &item = *control.item(i);
      item.setSelected(indexes.count(item.data(Qt::UserRole).toInt()) > 0);
    }
  }

  if (control.selectedItems().size() == control.count()) {
    control.setStyleSheet(styleSheet());
  } else {
    control.setStyleSheet("border-color:rgb(255,255,0);");
  }
}

void StrategyWindow::StoreConfig(const bool isActive) {
  m_engine.StoreConfig(*m_strategy, m_config, isActive);
}

void StrategyWindow::closeEvent(QCloseEvent *closeEvent) {
  if (!IsAutoTradingActivated()) {
    StoreConfig(false);
    closeEvent->accept();
    return;
  }
  const auto &response = QMessageBox::question(
      this, tr("Strategy closing"),
      tr("Are you sure you want to the close strategy window?\n\nIf strategy "
         "window will be closed - automatic trading will be stopped, all "
         "automatically opened position will not automatically closed.\n\n"
         "Continue and close strategy window?"),
      QMessageBox::Yes | QMessageBox::No);
  if (response == QMessageBox::Yes) {
    StoreConfig(false);
    closeEvent->accept();
  } else {
    closeEvent->ignore();
  }
}

bool StrategyWindow::IsAutoTradingActivated() const {
  return !m_strategy->IsBlocked() && m_strategy->IsTradingEnabled();
}
