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

MainWindow::MainWindow(QWidget *parent)
		: QMainWindow(parent),
		ui(new Ui::MainWindow),
		m_firstUpdateHistoryCounter(0) {
	
	ui->setupUi(this);

	{
		m_firstUpdateHistoryModel = new QStandardItemModel(0, 4, this);
		m_firstUpdateHistoryModel->setHorizontalHeaderItem(0, new QStandardItem(QString("#")));
		m_firstUpdateHistoryModel->setHorizontalHeaderItem(1, new QStandardItem(QString("price")));
		m_firstUpdateHistoryModel->setHorizontalHeaderItem(2, new QStandardItem(QString("size")));
		m_firstUpdateHistoryModel->setHorizontalHeaderItem(3, new QStandardItem(QString("side")));
		ui->firstUpdateHistory->setModel(m_firstUpdateHistoryModel);
	}

	m_updateTimer = new QTimer(this);
	connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(UpdateData()));

	connect(
		ui->symbol,
		SIGNAL(currentIndexChanged(const QString &)),
		this,
		SLOT(ClearSymbol()));

}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::ClearSymbol() {
	m_firstUpdateHistoryModel->setRowCount(1);
	m_firstUpdateHistoryCounter = 0;
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
	}
	m_firstUpdateHistoryCounter = 0;
	setCursor(Qt::ArrowCursor);
}

void MainWindow::Diconnect() {
	m_updateTimer->stop();
	m_service.reset();
	ui->serviceAddress->setEnabled(true);
	ui->connectButton->setText(tr("Connect"));
}

void MainWindow::on_connectButton_clicked() {
	!m_service ? Connect() : Diconnect();
}

void MainWindow::UpdateData() {
	if (!m_service || !ui->symbol->count()) {
		return;
	}
	ServiceAdapter::FirstUpdateData data;
	try {
		m_service->GetFirstUpdateData(ui->symbol->currentText(), data);
	} catch (const ServiceAdapter::Error &ex) {
		setCursor(Qt::ArrowCursor);
		QMessageBox::critical(
			this,
			tr("Service Error"),
			ex.what());
		Diconnect();
		return;
	}
	foreach (const auto &dataItem, data) {
		QList<QStandardItem *> historyItems;
		historyItems.push_back(
			new QStandardItem(QString("%1").arg(++m_firstUpdateHistoryCounter)));
		if (dataItem.price == 0) {
			historyItems.push_back(new QStandardItem(QString("-")));
		} else {
			historyItems.push_back(
				new QStandardItem(QString("%1").arg(double(dataItem.price) / 100)));
		}
		historyItems.push_back(
			new QStandardItem(QString("%1").arg(dataItem.qty)));
		historyItems.push_back(
			new QStandardItem(dataItem.isBuy ? tr("buy") : tr("sell")));
		m_firstUpdateHistoryModel->insertRow(0, historyItems);
		if (m_firstUpdateHistoryModel->rowCount() > 501) {
			m_firstUpdateHistoryModel->setRowCount(500);
		}
	}
}
