/**************************************************************************
 *   Created: 2012/09/25 23:00:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "ApplicationViewer.hpp"

//////////////////////////////////////////////////////////////////////////

class ApplicationViewer::Implementation {

public:

	QString m_mainQmlFile;

	QString adjustPath(const QString &path) const {
		return path;
	}

};

//////////////////////////////////////////////////////////////////////////

ApplicationViewer::ApplicationViewer(QWidget *parent)
		: QDeclarativeView(parent),
		m_piml(new Implementation) {
    connect(engine(), SIGNAL(quit()), SLOT(close()));
    setResizeMode(QDeclarativeView::SizeRootObjectToView);
}

ApplicationViewer::~ApplicationViewer() {
    delete m_piml;
}

ApplicationViewer *ApplicationViewer::create() {
    return new ApplicationViewer;
}

void ApplicationViewer::setMainQmlFile(const QString &file)
{
    m_piml->m_mainQmlFile = m_piml->adjustPath(file);
    setSource(QUrl::fromLocalFile(m_piml->m_mainQmlFile));
}

void ApplicationViewer::addImportPath(const QString &path)
{
    engine()->addImportPath(m_piml->adjustPath(path));
}

void ApplicationViewer::showExpanded() {
    show();
}

QApplication * createApplication(int &argc, char **argv) {
    return new QApplication(argc, argv);
}
