/*******************************************************************************
 *   Created: 2018/01/10 16:27:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "OrderStatusNotifier.hpp"

using namespace trdk;
using namespace trdk::FrontEnd::Lib;

void OrderStatusNotifier::OnOpen() {}

void OrderStatusNotifier::OnCancel() {}

void OrderStatusNotifier::OnTrade(const Trade &, bool) {}

void OrderStatusNotifier::OnReject() {}

void OrderStatusNotifier::OnError() {}

void OrderStatusNotifier::OnCommission(const Volume &) {}
