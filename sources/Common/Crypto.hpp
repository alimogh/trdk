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
std::vector<unsigned char> Decode(const std::string &);
}

std::string EncodeToHex(const unsigned char *cource, size_t sourceLen);

namespace Hmac {
boost::array<unsigned char, SHA512_DIGEST_LENGTH> CalcSha512Digest(
    const unsigned char *source, size_t sourceLen, const std::string &key);
boost::array<unsigned char, SHA512_DIGEST_LENGTH> CalcSha512Digest(
    const std::string &source, const std::string &key);

boost::array<unsigned char, SHA256_DIGEST_LENGTH> CalcSha256Digest(
    const unsigned char *source, size_t sourceLen, const std::string &key);
boost::array<unsigned char, SHA256_DIGEST_LENGTH> CalcSha256Digest(
    const std::string &source, const std::string &key);
boost::array<unsigned char, SHA256_DIGEST_LENGTH> CalcSha256Digest(
    const unsigned char *source,
    size_t sourceLen,
    const unsigned char *key,
    size_t keyLen);
boost::array<unsigned char, SHA256_DIGEST_LENGTH> CalcSha256Digest(
    const std::string &source, const unsigned char *key, size_t keyLen);
}
}
}
}