/*******************************************************************************
 *   Created: 2017/12/09 02:18:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "NonceStorage.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction::Rest;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

NonceStorage::Int32SecondsGenerator::Int32SecondsGenerator()
    : m_nextValue(static_cast<int32_t>((pt::second_clock::universal_time() -
                                        pt::ptime(gr::date(2018, 2, 18)))
                                           .total_seconds())) {}

NonceStorage::UnsignedInt64SecondsGenerator::UnsignedInt64SecondsGenerator()
    : m_nextValue((pt::microsec_clock::universal_time() -
                   pt::ptime(gr::date(2018, 2, 18)))
                      .total_microseconds()) {}

NonceStorage::UnsignedInt64MicrosecondsGenerator::
    UnsignedInt64MicrosecondsGenerator()
    : m_nextValue((pt::microsec_clock::universal_time() -
                   pt::ptime(gr::date(1970, 1, 1)))
                      .total_microseconds()) {}

NonceStorage::NonceStorage(std::unique_ptr<Generator>&& generator)
    : m_generator(std::move(generator)) {}

NonceStorage::TakenValue NonceStorage::TakeNonce() {
  Lock lock(m_mutex);
  return {m_generator->TakeNextNonce(), std::move(lock)};
}
