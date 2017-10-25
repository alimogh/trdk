/*******************************************************************************
 *   Created: 2017/10/15 22:36:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Style.hpp"

using namespace trdk::FrontEnd;

void Lib::LoadStyle(QApplication &application) {
  QFile qdarkstyleStyleFile(":qdarkstyle/style.qss");
  if (qdarkstyleStyleFile.exists()) {
    qdarkstyleStyleFile.open(QFile::ReadOnly | QFile::Text);
    application.setStyleSheet(QTextStream(&qdarkstyleStyleFile).readAll());
  }
}