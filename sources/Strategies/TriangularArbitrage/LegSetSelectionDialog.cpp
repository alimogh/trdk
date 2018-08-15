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
  {
    const auto &list =
        m_engine.GetContext().GetSettings().GetConfig().get_child_optional(
            "defaults.symbols");
    if (list) {
      for (const auto &node : *list) {
        const auto &symbol = node.second.get_value<std::string>();
        const auto delimeter = symbol.find('_');
        if (delimeter == std::string::npos) {
          continue;
        }
        const auto quoteSymbol =
            QString::fromStdString(symbol.substr(delimeter + 1)).trimmed();
        if (quoteSymbol.isEmpty()) {
          continue;
        }
        const auto baseSymbol =
            QString::fromStdString(symbol.substr(0, delimeter)).trimmed();
        if (baseSymbol.isEmpty()) {
          continue;
        }
        m_pairs.insert(QString::fromStdString(symbol),
                       std::make_pair(baseSymbol, quoteSymbol));
      }
    }
  }

  m_ui.setupUi(this);

  ResetLists();
  ConnectSiganls();
}

void LegSetSelectionDialog::ConnectSiganls() {
  Verify(connect(m_ui.leg1Buy, &QRadioButton::toggled, [this](bool isChecked) {
    if (!isChecked) {
      return;
    }
    UpdateSides(ORDER_SIDE_BUY, ORDER_SIDE_BUY, ORDER_SIDE_SELL);
  }));
  Verify(connect(m_ui.leg1Sell, &QRadioButton::toggled, [this](bool isChecked) {
    if (!isChecked) {
      return;
    }
    UpdateSides(ORDER_SIDE_SELL, ORDER_SIDE_SELL, ORDER_SIDE_BUY);
  }));
  Verify(connect(m_ui.leg2Buy, &QRadioButton::toggled, [this](bool isChecked) {
    if (!isChecked) {
      return;
    }
    UpdateSides(ORDER_SIDE_BUY, ORDER_SIDE_BUY, ORDER_SIDE_SELL);
  }));
  Verify(connect(m_ui.leg2Sell, &QRadioButton::toggled, [this](bool isChecked) {
    if (!isChecked) {
      return;
    }
    UpdateSides(ORDER_SIDE_SELL, ORDER_SIDE_SELL, ORDER_SIDE_BUY);
  }));
  Verify(connect(m_ui.leg3Buy, &QRadioButton::toggled, [this](bool isChecked) {
    if (!isChecked) {
      return;
    }
    UpdateSides(ORDER_SIDE_SELL, ORDER_SIDE_SELL, ORDER_SIDE_BUY);
  }));
  Verify(connect(m_ui.leg3Sell, &QRadioButton::toggled, [this](bool isChecked) {
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
  QListWidget *const controls[] = {m_ui.leg1Symbol, m_ui.leg2Symbol,
                                   m_ui.leg3Symbol};
  std::vector<QSignalBlocker> signalBlockers;

  for (auto &control : controls) {
    signalBlockers.emplace_back(*control);
    control->clear();
  }

  m_ui.leg2Symbol->setEnabled(false);
  m_ui.leg3Symbol->setEnabled(false);

  for (auto it = m_pairs.cbegin(); it != m_pairs.cend(); ++it) {
    const auto &leg1Pair = it.value();
    size_t numberOfLegs3 = 0;
    for (const auto &leg2Pair : m_pairs) {
      if (leg2Pair.first != leg1Pair.second) {
        continue;
      }
      for (const auto &leg3Pair : m_pairs) {
        if (leg3Pair.first != leg1Pair.first ||
            leg3Pair.second != leg2Pair.second) {
          continue;
        }
        ++numberOfLegs3;
      }
    }
    if (!numberOfLegs3) {
      continue;
    }
    m_ui.leg1Symbol->addItem(it.key());
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
  for (;;) {
    if (exec() != QDialog::Accepted) {
      return boost::none;
    }

    if (m_ui.leg1Symbol->currentRow() < 0 ||
        m_ui.leg2Symbol->currentRow() < 0 ||
        m_ui.leg3Symbol->currentRow() < 0) {
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
        LegConf{m_ui.leg3Symbol->currentItem()->text(),
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

  m_ui.leg2Symbol->setEnabled(true);
  m_ui.leg3Symbol->setEnabled(false);

  const auto &leg1Pair = m_ui.leg1Symbol->currentItem()->text().split("_");
  AssertEq(2, leg1Pair.size());
  if (leg1Pair.size() != 2) {
    return;
  }

  for (auto it = m_pairs.cbegin(); it != m_pairs.cend(); ++it) {
    if (it.value().first != leg1Pair[1]) {
      continue;
    }
    size_t numberOfLegs3 = 0;
    for (const auto &leg3Pair : m_pairs) {
      if (leg3Pair.first != leg1Pair[0] ||
          leg3Pair.second != it.value().second) {
        continue;
      }
      ++numberOfLegs3;
    }
    if (!numberOfLegs3) {
      continue;
    }
    AssertEq(1, numberOfLegs3);
    m_ui.leg2Symbol->addItem(it.key());
  }
  AssertLt(0, m_ui.leg2Symbol->count());
}

void LegSetSelectionDialog::UpdateSymbolsByLeg2() {
  const QSignalBlocker leg3SignalBlockers(m_ui.leg3Symbol);
  m_ui.leg3Symbol->clear();

  m_ui.leg3Symbol->setEnabled(true);

  const auto &leg1Pair = m_ui.leg1Symbol->currentItem()->text().split("_");
  AssertEq(2, leg1Pair.size());
  const auto &leg2Pair = m_ui.leg2Symbol->currentItem()->text().split("_");
  AssertEq(2, leg2Pair.size());
  if (leg1Pair.size() != 2 || leg2Pair.size() != 2) {
    return;
  }

  for (auto it = m_pairs.cbegin(); it != m_pairs.cend(); ++it) {
    if (it.value().first != leg1Pair[0] || it.value().second != leg2Pair[1]) {
      continue;
    }
    m_ui.leg3Symbol->addItem(it.key());
  }
  AssertEq(1, m_ui.leg3Symbol->count());

  m_ui.leg3Symbol->setCurrentRow(0);

  UpdateOkButton();
}

void LegSetSelectionDialog::UpdateOkButton() {
  m_ui.okButton->setEnabled(
      m_ui.leg3Symbol->currentRow() >= 0 &&
      m_ui.leg2Symbol->currentRow() >= 0 &&
      m_ui.leg1Symbol->currentRow() >= 0 &&
      (m_ui.leg1Buy->isChecked() || m_ui.leg1Sell->isChecked()) &&
      (m_ui.leg2Buy->isChecked() || m_ui.leg2Sell->isChecked()) &&
      (m_ui.leg3Buy->isChecked() || m_ui.leg3Sell->isChecked()));
}
