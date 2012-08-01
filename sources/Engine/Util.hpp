/**************************************************************************
 *   Created: 2012/07/22 23:40:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

class TradeSystem;
class IqFeedClient;
class Settings;

void Connect(TradeSystem &, const Settings &);
void Connect(IqFeedClient &, const Settings &);
