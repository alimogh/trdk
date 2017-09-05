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
#include "Shell.hpp"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[]) {
  QApplication application(argc, argv);
  application.setApplicationName(TRDK_NAME);

  Shell shell;
  shell.show();

  return application.exec();
}
