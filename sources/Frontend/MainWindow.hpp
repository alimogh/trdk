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
	void on_buyButton_clicked();
	void on_buyMktButton_clicked();
	void on_sellButton_clicked();
	void on_sellMktButton_clicked();
	void on_buyBidButton_clicked();
	void on_sellBidButton_clicked();
	void on_buyOfferButton_clicked();
	void on_sellOfferButton_clicked();
	void on_shortButton_clicked();

	void on_profitTargetResetButton_clicked();
	void on_stopLossResetButton_clicked();

	void UpdateData();
	void ChangeSymbol(const QString &symbols);

private:

	bool CheckPrice();
	bool CheckQty();

	void Connect();
	void Diconnect();

	void CreateCurrentBidAsk();

	void CreateTreades(QStandardItemModel *&model, QTableView &widget);
	void ClearTrades(QStandardItemModel &model);
	void UpdateTrades(const ServiceAdapter::Trades &, QStandardItemModel &model);

private:

	Ui::MainWindow *ui;

	QStandardItemModel *m_currentBidAskModel;
	QStandardItemModel *m_nasdaqTradesModel;
	QStandardItemModel *m_bxTradesModel;

	QTimer *m_updateTimer;

	std::unique_ptr<ServiceAdapter> m_service;
	ServiceAdapter::SecurityList m_securityList;

};
