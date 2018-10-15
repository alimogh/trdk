/*******************************************************************************
 *   Created: 2017/10/10 09:12:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Lib {
namespace Crypto {

namespace Base64 {
std::string Encode(const unsigned char *source,
                   size_t sourceLen,
                   bool multiline);
template <typename Source>
std::string Encode(const Source &source, const bool multiline) {
  return Encode(&source[0], source.size(), multiline);
}
std::vector<unsigned char> Decode(const std::string &);
}  // namespace Base64

std::string EncodeToHex(const unsigned char *source, size_t sourceLen);

namespace Hmac {
boost::array<unsigned char, 64> CalcSha512Digest(const unsigned char *source,
                                                 size_t sourceLen,
                                                 const std::string &key);
boost::array<unsigned char, 64> CalcSha512Digest(const std::string &source,
                                                 const std::string &key);
boost::array<unsigned char, 64> CalcSha512Digest(const unsigned char *source,
                                                 size_t sourceLen,
                                                 const unsigned char *key,
                                                 size_t keyLen);
boost::array<unsigned char, 64> CalcSha512Digest(const std::string &source,
                                                 const unsigned char *key,
                                                 size_t keyLen);
template <typename Key>
boost::array<unsigned char, 64> CalcSha512Digest(const std::string &source,
                                                 const Key &key) {
  return CalcSha512Digest(source, &key[0], key.size());
}

boost::array<unsigned char, 32> CalcSha256Digest(const unsigned char *source,
                                                 size_t sourceLen,
                                                 const std::string &key);
boost::array<unsigned char, 32> CalcSha256Digest(const std::string &source,
                                                 const std::string &key);
boost::array<unsigned char, 32> CalcSha256Digest(const unsigned char *source,
                                                 size_t sourceLen,
                                                 const unsigned char *key,
                                                 size_t keyLen);
boost::array<unsigned char, 32> CalcSha256Digest(const std::string &source,
                                                 const unsigned char *key,
                                                 size_t keyLen);
template <typename Key>
boost::array<unsigned char, 32> CalcSha256Digest(const std::string &source,
                                                 const Key &key) {
  return CalcSha256Digest(source, &key[0], key.size());
}
}  // namespace Hmac

boost::array<unsigned char, 16> CalcMd5(const std::string &);

boost::array<unsigned char, 32> CalcSha256(const std::string &);
}  // namespace Crypto
}  // namespace Lib
}  // namespace trdk