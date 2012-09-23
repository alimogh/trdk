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

private:

	Ui::MainWindow *ui;

	std::unique_ptr<ServiceAdapter> m_service;
	ServiceAdapter::SecurityList m_securityList;

};
