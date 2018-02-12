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

void OrderStatusNotifier::OnOpened() {}

void OrderStatusNotifier::OnTrade(const Trade &) {}

void OrderStatusNotifier::OnFilled(const Volume &) {}

void OrderStatusNotifier::OnCanceled(const Volume &) {}

void OrderStatusNotifier::OnRejected(const Volume &) {}

void OrderStatusNotifier::OnError(const Volume &) {}
