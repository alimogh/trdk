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
		ui(new Ui::MainWindow) {
	ui->setupUi(this);
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::on_connectButton_clicked() {
	
	setCursor(Qt::WaitCursor);
	
	if (!m_service) {
		m_service.reset(new ServiceAdapter(ui->serviceAddress->text()));
		try {
			m_service->GetSecurityList(m_securityList);
		} catch (const ServiceAdapter::Error &ex) {
			setCursor(Qt::ArrowCursor);
			QMessageBox::critical(
				this,
				tr("Service Error"),
				ex.what());
			return;
		}
		ui->serviceAddress->setEnabled(false);
		ui->symbol->clear();
		foreach (const auto &security, m_securityList) {
			ui->symbol->addItem(security.second.symbol);
		}
		ui->connectButton->setText(tr("Disconnect"));
	} else {
		m_service.reset();
		ui->serviceAddress->setEnabled(true);
		ui->connectButton->setText(tr("Connect"));
	}

	setCursor(Qt::ArrowCursor);

}
