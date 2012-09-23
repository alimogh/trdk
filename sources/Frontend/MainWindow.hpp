/**************************************************************************
 *   Created: 2012/09/23 20:27:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "ServiceAdapter.hpp"

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
		
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:

	void on_connectButton_clicked();
	void UpdateData();
	void ClearSymbol();

private:

	void Connect();
	void Diconnect();

private:

	Ui::MainWindow *ui;
	
	QStandardItemModel *m_firstUpdateHistoryModel;
	uint64_t m_firstUpdateHistoryCounter;

	QTimer *m_updateTimer;

	std::unique_ptr<ServiceAdapter> m_service;
	ServiceAdapter::SecurityList m_securityList;

};
