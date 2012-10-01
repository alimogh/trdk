/**************************************************************************
 *   Created: 2012/09/23 20:28:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include "ServiceAdapter.hpp"

namespace pt = boost::posix_time;
namespace lt = boost::local_time;

MainWindow::MainWindow(QWidget *parent)
		: QMainWindow(parent),
		ui(new Ui::MainWindow),
		m_nasdaqTradesModel(nullptr),
		m_bxTradesModel(nullptr) {
	
	ui->setupUi(this);

	CreateCurrentBidAsk();

	CreateTreades(m_nasdaqTradesModel, *ui->nasdaqTrades);
	CreateTreades(m_bxTradesModel, *ui->bxTrades);

	m_updateTimer = new QTimer(this);
	connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(UpdateData()));

	connect(
		ui->symbol,
		SIGNAL(currentIndexChanged(const QString &)),
		this,
		SLOT(ChangeSymbol(const QString &)));

#	ifdef DEV_VER
		ui->serviceAddress->setText("192.168.132.129:8080");
#	endif

	ui->profitTarget->clear();
	ui->stopLoss->clear();

}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::ChangeSymbol(const QString &symbols) {
	ClearTrades(*m_nasdaqTradesModel);
	ClearTrades(*m_bxTradesModel);
	if (m_service) {
		m_service->SetCurrentSymbol(symbols);
	}
}

void MainWindow::CreateCurrentBidAsk() {
	m_currentBidAskModel = new QStandardItemModel(0, 5, this);
	m_currentBidAskModel->setHorizontalHeaderItem(
		0,
		new QStandardItem(QString()));
	m_currentBidAskModel->setHorizontalHeaderItem(
		1,
		new QStandardItem(QString("Bid Size")));
	m_currentBidAskModel->setHorizontalHeaderItem(
		2,
		new QStandardItem(QString("Bid")));
	m_currentBidAskModel->setHorizontalHeaderItem(
		3,
		new QStandardItem(QString("Ask")));
	m_currentBidAskModel->setHorizontalHeaderItem(
		4,
		new QStandardItem(QString("Ask Side")));
	ui->currentBidAsk->setModel(m_currentBidAskModel);
	m_currentBidAskModel->appendRow(new QStandardItem(QString("NASDAQ")));
	m_currentBidAskModel->appendRow(new QStandardItem(QString("BX")));
	m_currentBidAskModel->appendRow(new QStandardItem(QString("NASDAQ")));
	m_currentBidAskModel->appendRow(new QStandardItem(QString("BX")));
}

void MainWindow::CreateTreades(QStandardItemModel *&model, QTableView &widget) {
	model = new QStandardItemModel(0, 3, this);
	model->setHorizontalHeaderItem(
		0,
		new QStandardItem(QString("Time")));
	model->setHorizontalHeaderItem(
		1,
		new QStandardItem(QString("Price")));
	model->setHorizontalHeaderItem(
		2,
		new QStandardItem(QString("Size")));
	model->setHorizontalHeaderItem(
		3,
		new QStandardItem(QString("Side")));
	widget.setModel(model);
}

void MainWindow::ClearTrades(QStandardItemModel &model) {
	model.setRowCount(0);
}

void MainWindow::Connect() {
	setCursor(Qt::WaitCursor);
	m_service.reset(new ServiceAdapter(ui->serviceAddress->text()));
	try {
		m_service->GetSecurityList(m_securityList);
	} catch (const ServiceAdapter::Error &ex) {
		setCursor(Qt::ArrowCursor);
		QMessageBox::critical(
			this,
			tr("Service Error"),
			ex.what());
		Diconnect();
		return;
	}
	ui->serviceAddress->setEnabled(false);
	ui->symbol->clear();
	foreach (const auto &security, m_securityList) {
		ui->symbol->addItem(security.second.symbol);
	}
	ui->connectButton->setText(tr("Disconnect"));
	bool isEnabled = ui->symbol->count() > 0;
	if (isEnabled) {
		m_updateTimer->start(500);
	} else {
		m_updateTimer->stop();
	}

	ui->symbol->setEnabled(isEnabled);
	ui->pauseButton->setEnabled(isEnabled);
	ui->price->setEnabled(isEnabled);
	ui->qty->setEnabled(isEnabled);
	ui->profitTarget->setEnabled(isEnabled);
	ui->profitTargetResetButton->setEnabled(isEnabled);
	ui->stopLoss->setEnabled(isEnabled);
	ui->stopLossResetButton->setEnabled(isEnabled);
	ui->buyButton->setEnabled(isEnabled);
	ui->sellButton->setEnabled(isEnabled);
	ui->shortButton->setEnabled(isEnabled);
	ui->buyMktButton->setEnabled(isEnabled);
	ui->buyBidButton->setEnabled(isEnabled);
	ui->sellMktButton->setEnabled(isEnabled);
	ui->sellBidButton->setEnabled(isEnabled);
	ui->sellOfferButton->setEnabled(isEnabled);
	ui->buyOfferButton->setEnabled(isEnabled);

	setCursor(Qt::ArrowCursor);
}

void MainWindow::Diconnect() {
	
	m_updateTimer->stop();
	
	ui->symbol->setEnabled(false);
	ui->pauseButton->setEnabled(false);
	ui->price->setEnabled(false);
	ui->qty->setEnabled(false);
	ui->profitTarget->setEnabled(false);
	ui->profitTargetResetButton->setEnabled(false);
	ui->stopLoss->setEnabled(false);
	ui->stopLossResetButton->setEnabled(false);
	ui->buyButton->setEnabled(false);
	ui->sellButton->setEnabled(false);
	ui->shortButton->setEnabled(false);
	ui->buyMktButton->setEnabled(false);
	ui->buyBidButton->setEnabled(false);
	ui->sellMktButton->setEnabled(false);
	ui->sellBidButton->setEnabled(false);
	ui->sellOfferButton->setEnabled(false);
	ui->buyOfferButton->setEnabled(false);
	
	m_service.reset();
	
	ui->price->clear();
	ui->serviceAddress->setEnabled(true);
	ui->connectButton->setText(tr("Connect"));

}

void MainWindow::on_connectButton_clicked() {
	!m_service ? Connect() : Diconnect();
}

void MainWindow::on_pauseButton_clicked() {
	if (!m_service) {
		return;
	}
	!ui->pauseButton->isChecked()
		?	m_updateTimer->start(500)
		:	m_updateTimer->stop();
}

namespace {
	boost::shared_ptr<lt::posix_time_zone> GetEdtTimeZone() {
		static boost::shared_ptr<lt::posix_time_zone> edtTimeZone(
			new lt::posix_time_zone("EST-05EDT+01,M4.1.0/02:00,M10.5.0/02:00"));
		return edtTimeZone;
	}
	pt::time_duration GetEdtDiff() {
		const lt::local_date_time estTime(boost::get_system_time(), GetEdtTimeZone());
		return estTime.local_time() - estTime.utc_time();
	}
}

void MainWindow::UpdateTrades(
			const ServiceAdapter::Trades &trades,
			QStandardItemModel &model) {

	foreach (const auto &trade, trades) {
	
		QList<QStandardItem *> historyItems;
		{
			std::ostringstream oss;
			auto time = pt::from_time_t(trade.time.date);
			time += pt::milliseconds(trade.time.time);
			time += GetEdtDiff();
			oss << time.time_of_day();
			historyItems.push_back(
				new QStandardItem(QString::fromStdString(oss.str())));
		}
		if (trade.param.price == 0) {
			historyItems.push_back(new QStandardItem(QString("-")));
		} else {
			historyItems.push_back(
				new QStandardItem(m_service->DescalePriceAndConvert(trade.param.price)));
		}
		historyItems.push_back(
			new QStandardItem(QString("%1").arg(trade.param.qty)));
		historyItems.push_back(
			new QStandardItem(trade.isBuy ? tr("Bought") : tr("Sold")));
		
		model.appendRow(historyItems);
		const auto row = model.rowCount() - 1;
		for (auto i = 0; i < model.columnCount(); ++i) {
			model.setData(
				model.index(row, i, QModelIndex()),
				!trade.isBuy ? QColor(255, 204, 204) : QColor(204, 255, 204),
				Qt::BackgroundRole);
			if (row < 50) {
				ui->nasdaqTrades->resizeColumnToContents(i);
			}
		}

	}

}

void MainWindow::UpdateData() {
	
	if (!m_service || !ui->symbol->count()) {
		return;
	}
	
	try {

		{
			ServiceAdapter::Trades nasdaqTrades;
			m_service->GetLastNasdaqTrades(nasdaqTrades);
			UpdateTrades(nasdaqTrades, *m_nasdaqTradesModel);
		}

		{
			ServiceAdapter::Trades bxTrades;
			m_service->GetLastBxTrades(bxTrades);
			UpdateTrades(bxTrades, *m_bxTradesModel);
		}

		{
			ServiceAdapter::CommonParams params;
			m_service->GetCommonParams(params);
			ui->lastTradePrice->setValue(m_service->DescalePrice(params.last.price));
			ui->lastTradeQty->setValue(int(params.last.qty));
			ui->bestBidPrice->setValue(m_service->DescalePrice(params.best.bid.price));
			ui->bestAskPrice->setValue(m_service->DescalePrice(params.best.ask.price));
			ui->volumeTraded->setValue(int(params.volumeTraded));
		}

		{
			ServiceAdapter::ExchangeBook book;
			m_service->GetNasdaqBook(book);
			m_currentBidAskModel->setItem(
				0,
				1,
				new QStandardItem(QString("%1").arg(book.params1.bid.qty)));
			m_currentBidAskModel->setItem(
				0,
				2,
				new QStandardItem(m_service->DescalePriceAndConvert(book.params1.bid.price)));
			m_currentBidAskModel->setItem(
				0,
				3,
				new QStandardItem(m_service->DescalePriceAndConvert(book.params1.ask.price)));
			m_currentBidAskModel->setItem(
				0,
				4,
				new QStandardItem(QString("%1").arg(book.params1.ask.qty)));
			m_currentBidAskModel->setItem(
				2,
				1,
				new QStandardItem(QString("%1").arg(book.params2.bid.qty)));
			m_currentBidAskModel->setItem(
				2,
				2,
				new QStandardItem(m_service->DescalePriceAndConvert(book.params2.bid.price)));
			if (book.params2.ask.price || book.params2.ask.qty) {
				m_currentBidAskModel->setItem(
					2,
					3,
					new QStandardItem(m_service->DescalePriceAndConvert(book.params2.ask.price)));
				m_currentBidAskModel->setItem(
					2,
					4,
					new QStandardItem(QString("%1").arg(book.params2.ask.qty)));
			} else {
				m_currentBidAskModel->setItem(2, 3, new QStandardItem(QString()));
				m_currentBidAskModel->setItem(2, 4, new QStandardItem(QString()));
			}
			for (auto i = 0; i < m_currentBidAskModel->columnCount(); ++i) {
				ui->currentBidAsk->resizeColumnToContents(i);
			}
		}

	} catch (const ServiceAdapter::Error &ex) {
		setCursor(Qt::ArrowCursor);
		QMessageBox::critical(this, tr("Service Error"), ex.what());
		Diconnect();
		return;
	}

}

bool MainWindow::CheckPrice() {
	if (!(ui->price->value() > 0)) {
		ui->price->setFocus(Qt::OtherFocusReason);
		ui->price->selectAll();
		QMessageBox::critical(this, tr("Order Error"), tr("Please provide an order price."));
		return false;
	}
	return true;
}
bool MainWindow::CheckQty() {
	if (!(ui->qty->value() > 0)) {
		ui->qty->setFocus(Qt::OtherFocusReason);
		ui->qty->selectAll();
		QMessageBox::critical(this, tr("Order Error"), tr("Please provide an order quantity."));
		return false;
	}
	return true;
}

void MainWindow::on_shortButton_clicked() {
	on_sellButton_clicked();
}

void MainWindow::on_buyButton_clicked() {
	if (!CheckPrice() || !CheckQty()) {
		return;
	}
	try {
		m_service->OrderBuy(ui->price->value(), ui->qty->value());
	} catch (const ServiceAdapter::Error &ex) {
		setCursor(Qt::ArrowCursor);
		QMessageBox::critical(this, tr("Service Error"), ex.what());
		Diconnect();
	}
}

void MainWindow::on_buyMktButton_clicked() {
	if (!CheckQty()) {
		return;
	}
	try {
		m_service->OrderBuyMkt(ui->qty->value());
	} catch (const ServiceAdapter::Error &ex) {
		setCursor(Qt::ArrowCursor);
		QMessageBox::critical(this, tr("Service Error"), ex.what());
		Diconnect();
	}
}

void MainWindow::on_sellButton_clicked() {
	if (!CheckPrice() || !CheckQty()) {
		return;
	}
	try {
		m_service->OrderSell(ui->price->value(), ui->qty->value());
	} catch (const ServiceAdapter::Error &ex) {
		setCursor(Qt::ArrowCursor);
		QMessageBox::critical(this, tr("Service Error"), ex.what());
		Diconnect();
	}
}

void MainWindow::on_sellMktButton_clicked() {
	if (!m_service || !CheckQty()) {
		return;
	}
	try {
		m_service->OrderSellMkt(ui->qty->value());
	} catch (const ServiceAdapter::Error &ex) {
		setCursor(Qt::ArrowCursor);
		QMessageBox::critical(this, tr("Service Error"), ex.what());
		Diconnect();
	}
}

void MainWindow::on_buyBidButton_clicked() {
	if (!CheckQty()) {
		return;
	}
	ui->price->setValue(ui->bestBidPrice->value());
	on_buyButton_clicked();
}

void MainWindow::on_sellBidButton_clicked() {
	if (!CheckQty()) {
		return;
	}
	ui->price->setValue(ui->bestBidPrice->value());
	on_sellButton_clicked();
}

void MainWindow::on_buyOfferButton_clicked() {
	if (!CheckQty()) {
		return;
	}
	ui->price->setValue(ui->bestAskPrice->value());
	on_buyButton_clicked();
}

void MainWindow::on_sellOfferButton_clicked() {
	if (!CheckQty()) {
		return;
	}
	ui->price->setValue(ui->bestAskPrice->value());
	on_sellButton_clicked();
}

void MainWindow::on_profitTargetResetButton_clicked() {
	ui->profitTarget->clear();
}

void MainWindow::on_stopLossResetButton_clicked() {
	ui->stopLoss->clear();
}
