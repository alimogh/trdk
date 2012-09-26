/**************************************************************************
 *   Created: 2012/09/23 20:28:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "MainWindow.hpp"

int main(int argc, char *argv[]) {

	QApplication a(argc, argv);
	MainWindow w;
	w.show();

	return a.exec();

}
