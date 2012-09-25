/**************************************************************************
 *   Created: 2012/09/23 20:28:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "ApplicationViewer.hpp"

int main(int argc, char *argv[]) {

	QScopedPointer<QApplication> app(createApplication(argc, argv));

	ApplicationViewer viewer;
	viewer.setMainQmlFile(QLatin1String("main.qml"));
	viewer.showExpanded();

	return app->exec();

}
