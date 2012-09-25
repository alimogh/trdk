/**************************************************************************
 *   Created: 2012/09/25 22:59:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

class ApplicationViewer : public QDeclarativeView {

	Q_OBJECT

public:
	
	enum ScreenOrientation {
		ScreenOrientationLockPortrait,
		ScreenOrientationLockLandscape,
		ScreenOrientationAuto
	};

public:

	explicit ApplicationViewer(QWidget *parent = nullptr);
	virtual ~ApplicationViewer();

public:

	static ApplicationViewer * create();

	void setMainQmlFile(const QString &file);
	void addImportPath(const QString &path);

	void showExpanded();

private:

	class Implementation;
	Implementation *m_piml;

};

QApplication * createApplication(int &argc, char **argv);
