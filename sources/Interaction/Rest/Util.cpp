/*******************************************************************************
 *   Created: 2017/11/11 13:44:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;

namespace is = boost::iostreams;
namespace ptr = boost::property_tree;

ptr::ptree Rest::ReadJson(const std::string &source) {
  is::stream_buffer<is::array_source> buffer(source.c_str(),
                                             source.c_str() + source.size());
  std::istream stream(&buffer);
  ptr::ptree result;
  ptr::read_json(stream, result);
  return result;
}
