/*******************************************************************************
 *   Created: 2018/01/31 23:55:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "LegSetSelectionDialog.hpp"

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;
using namespace Strategies::TriangularArbitrage;

LegSetSelectionDialog::LegSetSelectionDialog(Engine &engine, QWidget *parent)
    : QDialog(parent), m_engine(engine) {
  m_ui.setupUi(this);
  setEnabled(false);
  ConnectSiganls();
}

void LegSetSelectionDialog::LoadPairs() {
  typedef boost::unordered_map<std::string, std::pair<std::string, std::string>>
      Pairs;
  Pairs pairs;
  boost::unordered_map<std::string, std::vector<Pairs::const_iterator>>
      baseSymbols;

  for (auto symbol : m_engine.GetContext().GetSymbolListHint()) {
    const auto delimeter = symbol.find('_');
    if (delimeter == std::string::npos) {
      continue;
    }

    auto quoteSymbol = symbol.substr(delimeter + 1);
    boost::trim(quoteSymbol);
    if (quoteSymbol.empty()) {
      continue;
    }

    auto baseSymbol = symbol.substr(0, delimeter);
    boost::trim(baseSymbol);
    if (baseSymbol.empty()) {
      continue;
    }

    const auto &it = pairs.emplace(
        std::move(symbol),
        std::make_pair(std::move(baseSymbol), std::move(quoteSymbol)));
    if (!it.second) {
      continue;
    }

    baseSymbols[it.first->second.first].emplace_back(it.first);
  }

  for (const auto &leg1Pair : pairs) {
    const auto &leg1QuoteSymbol = leg1Pair.second.second;
    const auto &leg2BaseSymbols = baseSymbols.find(leg1QuoteSymbol);
    if (leg2BaseSymbols == baseSymbols.cend()) {
      continue;
    }

    std::vector<QString> leg2Pairs;
    for (const auto &leg2Pair : leg2BaseSymbols->second) {
      auto leg3Pair = leg1QuoteSymbol;
      leg3Pair += '_';
      leg3Pair += leg2Pair->second.second;
      if (pairs.count(leg3Pair)) {
        leg2Pairs.emplace_back(QString::fromStdString(leg2Pair->first));
      }
    }

    if (leg2Pairs.empty()) {
      continue;
    }

    auto symbol = QString::fromStdString(leg1Pair.first);
    m_leg2Pairs.insert(symbol, leg2Pairs);
    m_leg1Pairs.emplace_back(std::move(symbol));
  }
}

void LegSetSelectionDialog::ConnectSiganls() {
  Verify(connect(m_ui.leg1SymbolFilter, &QLineEdit::textEdited,
                 [this]() { FilterLeg1Symbols(); }));

  Verify(connect(m_ui.leg1Buy, &QRadioButton::toggled,
                 [this](const bool isChecked) {
                   if (!isChecked) {
                     return;
                   }
                   UpdateSides(ORDER_SIDE_BUY, ORDER_SIDE_BUY, ORDER_SIDE_SELL);
                 }));
  Verify(connect(
      m_ui.leg1Sell, &QRadioButton::toggled, [this](const bool isChecked) {
        if (!isChecked) {
          return;
        }
        UpdateSides(ORDER_SIDE_SELL, ORDER_SIDE_SELL, ORDER_SIDE_BUY);
      }));
  Verify(connect(m_ui.leg2Buy, &QRadioButton::toggled,
                 [this](const bool isChecked) {
                   if (!isChecked) {
                     return;
                   }
                   UpdateSides(ORDER_SIDE_BUY, ORDER_SIDE_BUY, ORDER_SIDE_SELL);
                 }));
  Verify(connect(
      m_ui.leg2Sell, &QRadioButton::toggled, [this](const bool isChecked) {
        if (!isChecked) {
          return;
        }
        UpdateSides(ORDER_SIDE_SELL, ORDER_SIDE_SELL, ORDER_SIDE_BUY);
      }));
  Verify(connect(
      m_ui.leg3Buy, &QRadioButton::toggled, [this](const bool isChecked) {
        if (!isChecked) {
          return;
        }
        UpdateSides(ORDER_SIDE_SELL, ORDER_SIDE_SELL, ORDER_SIDE_BUY);
      }));
  Verify(connect(m_ui.leg3Sell, &QRadioButton::toggled,
                 [this](const bool isChecked) {
                   if (!isChecked) {
                     return;
                   }
                   UpdateSides(ORDER_SIDE_BUY, ORDER_SIDE_BUY, ORDER_SIDE_SELL);
                 }));

  Verify(connect(m_ui.leg1Symbol, &QListWidget::currentItemChanged,
                 [this]() { UpdateSymbolsByLeg1(); }));
  Verify(connect(m_ui.leg2Symbol, &QListWidget::currentItemChanged,
                 [this]() { UpdateSymbolsByLeg2(); }));
  Verify(connect(m_ui.leg2Symbol, &QListWidget::currentItemChanged,
                 [this]() { UpdateOkButton(); }));
}

void LegSetSelectionDialog::ResetLists() {
  QListWidget *const controls[] = {m_ui.leg1Symbol, m_ui.leg2Symbol};
  std::vector<QSignalBlocker> signalBlockers;

  for (auto &control : controls) {
    signalBlockers.emplace_back(*control);
    control->clear();
  }
  signalBlockers.emplace_back(m_ui.leg3Symbol);
  m_ui.leg3Symbol->clear();

  m_ui.exchangeList->clear();

  m_ui.leg2Symbol->setEnabled(false);
  m_ui.leg3Symbol->setEnabled(false);

  for (const auto &pair : m_leg1Pairs) {
    m_ui.leg1Symbol->addItem(pair);
  }

  if (!m_ui.leg1Symbol->count()) {
    QMessageBox::warning(
        this, tr("Symbol list is empty"),
        tr("There are no any suitable pairs to set up this strategy. Connect "
           "more exchanges to expand the pair list."),
        QMessageBox::Ok);
    m_ui.leg1Symbol->setEnabled(false);
  }

  UpdateOkButton();
}

boost::optional<LegsConf> LegSetSelectionDialog::RequestLegSet() {
  if (!isEnabled()) {
    LoadPairs();
    setEnabled(true);
    ResetLists();
  }

  for (;;) {
    if (exec() != QDialog::Accepted) {
      return boost::none;
    }

    if (m_ui.leg1Symbol->currentRow() < 0 ||
        m_ui.leg2Symbol->currentRow() < 0) {
      QMessageBox::warning(this, tr("Pair is not set"),
                           tr("Select pair and order side for each leg."),
                           QMessageBox::Ok);
      m_ui.okButton->setEnabled(false);
      continue;
    }

    return LegsConf{
        LegConf{m_ui.leg1Symbol->currentItem()->text(),
                m_ui.leg1Buy->isChecked() ? ORDER_SIDE_BUY : ORDER_SIDE_SELL},
        LegConf{m_ui.leg2Symbol->currentItem()->text(),
                m_ui.leg2Buy->isChecked() ? ORDER_SIDE_BUY : ORDER_SIDE_SELL},
        LegConf{m_ui.leg3Symbol->text(),
                m_ui.leg3Buy->isChecked() ? ORDER_SIDE_BUY : ORDER_SIDE_SELL}};
  }
}

void LegSetSelectionDialog::UpdateSides(const OrderSide &leg1Side,
                                        const OrderSide &leg2Side,
                                        const OrderSide &leg3Side) {
  const QSignalBlocker signalBlockers[] = {
      QSignalBlocker{m_ui.leg1Buy}, QSignalBlocker{m_ui.leg1Sell},
      QSignalBlocker{m_ui.leg2Buy}, QSignalBlocker{m_ui.leg2Sell},
      QSignalBlocker{m_ui.leg3Buy}, QSignalBlocker{m_ui.leg3Sell}};

  m_ui.leg1Buy->setChecked(leg1Side == ORDER_SIDE_BUY);
  m_ui.leg1Sell->setChecked(leg1Side == ORDER_SIDE_SELL);

  m_ui.leg2Buy->setChecked(leg2Side == ORDER_SIDE_BUY);
  m_ui.leg2Sell->setChecked(leg2Side == ORDER_SIDE_SELL);

  m_ui.leg3Buy->setChecked(leg3Side == ORDER_SIDE_BUY);
  m_ui.leg3Sell->setChecked(leg3Side == ORDER_SIDE_SELL);

  UpdateOkButton();
}

void LegSetSelectionDialog::UpdateSymbolsByLeg1() {
  const QSignalBlocker leg2SignalBlockers(m_ui.leg2Symbol);
  const QSignalBlocker leg3SignalBlockers(m_ui.leg3Symbol);
  m_ui.leg2Symbol->clear();
  m_ui.leg3Symbol->clear();
  m_ui.exchangeList->clear();

  m_ui.leg2Symbol->setEnabled(true);
  m_ui.leg3Symbol->setEnabled(false);

  const auto &leg2Pairs =
      m_leg2Pairs.find(m_ui.leg1Symbol->currentItem()->text());
  Assert(leg2Pairs != m_leg2Pairs.cend());
  if (leg2Pairs == m_leg2Pairs.cend()) {
    return;
  }

  for (const auto &leg2Pair : leg2Pairs.value()) {
    m_ui.leg2Symbol->addItem(leg2Pair);
  }
  AssertLt(0, m_ui.leg2Symbol->count());
}

void LegSetSelectionDialog::UpdateSymbolsByLeg2() {
  const auto &leg1Pair = m_ui.leg1Symbol->currentItem()->text().split("_");
  AssertEq(2, leg1Pair.size());
  const auto &leg2Pair = m_ui.leg2Symbol->currentItem()->text().split("_");
  AssertEq(2, leg2Pair.size());
  if (leg1Pair.size() != 2 || leg2Pair.size() != 2) {
    return;
  }

  const QSignalBlocker leg3SignalBlockers(m_ui.leg3Symbol);
  m_ui.leg3Symbol->clear();
  m_ui.exchangeList->clear();
  m_ui.leg3Symbol->setEnabled(true);
  m_ui.leg3Symbol->setText(leg1Pair[0] + "_" + leg2Pair[1]);

  {
    const auto &leg1PairStr =
        m_ui.leg1Symbol->currentItem()->text().toStdString();
    const auto &leg2PairStr =
        m_ui.leg2Symbol->currentItem()->text().toStdString();
    const auto &leg3PairStr = m_ui.leg3Symbol->text().toStdString();
    m_engine.GetContext().ForEachMarketDataSource(
        [&](const MarketDataSource &source) {
          const auto &symbols = source.GetSymbolListHint();
          if (symbols.count(leg1PairStr) && symbols.count(leg2PairStr) &&
              symbols.count(leg3PairStr)) {
            m_ui.exchangeList->addItem(
                QString::fromStdString(source.GetTitle()));
          }
        });
  }

  UpdateOkButton();
}

void LegSetSelectionDialog::UpdateOkButton() {
  m_ui.okButton->setEnabled(
      m_ui.leg2Symbol->currentRow() >= 0 &&
      m_ui.leg1Symbol->currentRow() >= 0 &&
      (m_ui.leg1Buy->isChecked() || m_ui.leg1Sell->isChecked()) &&
      (m_ui.leg2Buy->isChecked() || m_ui.leg2Sell->isChecked()) &&
      (m_ui.leg3Buy->isChecked() || m_ui.leg3Sell->isChecked()));
}

void LegSetSelectionDialog::FilterLeg1Symbols() {
  QString selection;
  if (m_ui.leg1Symbol->currentItem()) {
    selection = m_ui.leg1Symbol->currentItem()->text();
  }

  QSignalBlocker blocker(m_ui.leg1Symbol);

  m_ui.leg1Symbol->clear();

  const auto &filter = m_ui.leg1SymbolFilter->text();
  auto row = -1;
  for (const auto &symbol : m_leg1Pairs) {
    if (!symbol.contains(filter, Qt::CaseInsensitive)) {
      continue;
    }
    m_ui.leg1Symbol->addItem(symbol);
    if (selection == symbol) {
      row = m_ui.leg1Symbol->count() - 1;
    }
  }

  if (!selection.isEmpty() && row == -1) {
    blocker.unblock();
  }

  m_ui.leg1Symbol->setCurrentRow(row);
}
