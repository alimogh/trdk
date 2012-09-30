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
	void on_pauseButton_clicked();
	void UpdateData();
	void ChangeSymbol(const QString &symbols);

private:

	void Connect();
	void Diconnect();

	void CreateCurrentBidAsk();

	void CreateOrders(QStandardItemModel *&model, QTableView &widget);
	void ClearTrades(QStandardItemModel &model);
	void UpdateTrades(const ServiceAdapter::Orders &, QStandardItemModel &model);

private:

	Ui::MainWindow *ui;

	QStandardItemModel *m_currentBidAskModel;
	QStandardItemModel *m_nasdaqOrdersModel;
	QStandardItemModel *m_bxOrdersModel;

	QTimer *m_updateTimer;

	std::unique_ptr<ServiceAdapter> m_service;
	ServiceAdapter::SecurityList m_securityList;

};
