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

std::string Rest::ConvertToString(const ptr::ptree &source, bool multiline) {
  std::stringstream ss;
  boost::property_tree::json_parser::write_json(ss, source, multiline);
  auto result = ss.str();
  if (!multiline) {
    boost::replace_all(result, "\n", " ");
  }
  boost::trim(result);
  return result;
}
