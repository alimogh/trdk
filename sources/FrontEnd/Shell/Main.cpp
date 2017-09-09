/*******************************************************************************
 *   Created: 2017/09/05 08:19:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MainWindow.hpp"

using namespace trdk::Frontend::Shell;

int main(int argc, char *argv[]) {
  QApplication application(argc, argv);
  application.setApplicationName(TRDK_NAME);
  {
    QFile qdarkstyleStyleFile(":qdarkstyle/style.qss");
    if (qdarkstyleStyleFile.exists()) {
      qdarkstyleStyleFile.open(QFile::ReadOnly | QFile::Text);
      application.setStyleSheet(QTextStream(&qdarkstyleStyleFile).readAll());
    }
  }

  MainWindow mainWindow;
  mainWindow.show();

  return application.exec();
}
