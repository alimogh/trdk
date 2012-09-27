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
	if (ui->symbol->count() > 0) {
		m_updateTimer->start(500);
		ui->pauseButton->setEnabled(true);
	} else {
		m_updateTimer->stop();
		ui->pauseButton->setEnabled(false);
	}
	setCursor(Qt::ArrowCursor);
}

void MainWindow::Diconnect() {
	m_updateTimer->stop();
	ui->pauseButton->setEnabled(false);
	m_service.reset();
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
				new QStandardItem(m_service->DescaleAndConvert(trade.param.price)));
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
			ui->lastTradePrice->setText(
				m_service->DescaleAndConvert(params.last.price));
			ui->lastTradeQty->setText(QString("%1").arg(params.last.qty));
			ui->bestBidPrice->setText(
				m_service->DescaleAndConvert(params.best.bid.price));
			ui->bestAskPrice->setText(
				m_service->DescaleAndConvert(params.best.ask.price));
			ui->volumeTraded->setText(QString("%1").arg(params.volumeTraded));
		}

		{
			ServiceAdapter::ExchangeParams params;
			m_service->GetNasdaqParams(params);
			m_currentBidAskModel->setItem(
				0,
				1,
				new QStandardItem(QString("%1").arg(params.bid.qty)));
			m_currentBidAskModel->setItem(
				0,
				2,
				new QStandardItem(m_service->DescaleAndConvert(params.bid.price)));
			m_currentBidAskModel->setItem(
				0,
				3,
				new QStandardItem(m_service->DescaleAndConvert(params.ask.price)));
			m_currentBidAskModel->setItem(
				0,
				4,
				new QStandardItem((QString("%1").arg(params.ask.qty))));
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
