//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

namespace trdk {
namespace Lib {
namespace Details {

//! Combines value hashes.
/**
 * Code from boost
 * Reciprocal of the golden ratio helps spread entropy and handles duplicates.
 * See Mike Seymour in magic-numbers-in-boosthash-combine:
 *     http://stackoverflow.com/questions/4948780
 */
template <typename T>
void CombineHash(const T& value, std::size_t& result) {
  result ^= std::hash<T>()(value) + 0x9e3779b9 + (result << 6) + (result >> 2);
}

//! Recursive template code derived from Matthieu M.
template <typename Tuple, size_t index>
struct HashTupleVisitor {
  static void apply(const Tuple& tuple, size_t& result) {
    HashTupleVisitor<Tuple, index - 1>::apply(tuple, result);
    CombineHash(boost::get<index>(tuple), result);
  }
};

template <class Tuple>
struct HashTupleVisitor<Tuple, 0> {
  static void apply(const Tuple& tuple, size_t& result) {
    CombineHash(boost::get<0>(tuple), result);
  }
};

template <typename T>
std::size_t HashTuple(const T& value) {
  size_t result = 0;
  HashTupleVisitor<T, boost::tuples::length<T>::value - 1>::apply(value,
                                                                  result);
  return result;
}

}  // namespace Details
}  // namespace Lib

template <typename... TT>
std::size_t hash_value(const boost::tuple<TT...>& value) {
  return Lib::Details::HashTuple(value);
}

}  // namespace trdk
