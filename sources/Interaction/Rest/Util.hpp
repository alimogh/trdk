/*******************************************************************************
 *   Created: 2017/10/30 22:56:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Interaction {
namespace Rest {

std::string ConvertToString(const boost::property_tree::ptree &,
                            bool multiline);
boost::property_tree::ptree ReadJson(const std::string &);

std::unique_ptr<Poco::Net::HTTPClientSession> CreateSession(
    const std::string &host, const Settings &);
}
}
}
