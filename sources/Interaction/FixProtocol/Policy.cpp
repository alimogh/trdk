/*******************************************************************************
 *   Created: 2017/10/01 06:47:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Policy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::FixProtocol;

Policy::Policy(const trdk::Settings &settings)
    : m_utcDiff(settings.GetTimeZone()->base_utc_offset()) {}
