/**************************************************************************
 *   Created: 2016/12/15 04:47:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "StopOrder.hpp"
#include "PositionController.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

StopOrder::StopOrder(Position &position, PositionController &controller)
    : Algo(position), m_controller(controller) {}

void StopOrder::OnHit(const CloseReason &reason) {
  try {
    m_controller.Close(GetPosition(), reason);
  } catch (const CommunicationError &ex) {
    GetPosition().GetStrategy().GetLog().Warn(
        "Communication error while starting position closing by stop-order "
        "\"%1%\": \"%2%\".",
        GetName(),   // 1
        ex.what());  // 2
  } catch (const std::exception &ex) {
    GetPosition().GetStrategy().GetLog().Error(
        "Failed to start position closing by stop-order \"%1%\": \"%2%\".",
        GetName(),   // 1
        ex.what());  // 2
    throw;
  }
}
