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
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

namespace fs = boost::filesystem;

NonceStorage::NonceStorage(const Settings &settings, ModuleEventsLog &log) {
  {
    std::ifstream nonceStorage(settings.nonceStorageFile.string().c_str());
    if (!nonceStorage) {
      log.Debug("Failed to open %1% to read.", settings.nonceStorageFile);
    } else {
      nonceStorage.read(reinterpret_cast<char *>(&m_value), sizeof(m_value));
      log.Debug("Restored %1%nonce-value %2% from %3%.",
                !settings.isTrading ? "" : "trading ",  // 1
                m_value,                                // 2
                settings.nonceStorageFile);             // 3
    }
    if (m_value < settings.initialNonce) {
      log.Debug("Using %1%initial nonce-value %2%.",
                !settings.isTrading ? "" : "trading ",  // 1
                settings.initialNonce);                 // 2
      m_value = settings.initialNonce;
    }
  }
  {
    fs::create_directories(settings.nonceStorageFile.branch_path());
    m_storage = std::ofstream(settings.nonceStorageFile.string().c_str(),
                              std::fstream::out | std::fstream::binary);
    if (!m_storage) {
      boost::format error("Failed to open %1% to store");
      error % settings.nonceStorageFile;
      throw Exception(error.str().c_str());
    }
  }
}

NonceStorage::TakenValue NonceStorage::TakeNonce() {
  Lock lock(m_mutex);
  m_value++;
  m_storage.seekp(0);
  m_storage.write(reinterpret_cast<const char *>(&m_value), sizeof(m_value));
  m_storage.flush();
  AssertEq(sizeof(m_value), m_storage.tellp());
  return {m_value - 1, std::move(lock)};
}