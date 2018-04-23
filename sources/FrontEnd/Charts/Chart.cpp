//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "Chart.hpp"

using namespace trdk::FrontEnd::Charts;

Chart::Chart(QGraphicsItem *parent) : Base(parent) {
  setAnimationOptions(QChart::AllAnimations);
}
