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

class Base64Coder : private boost::noncopyable {
 public:
  explicit Base64Coder(bool multiline);
  ~Base64Coder();

 public:
  void Encode(const unsigned char *source, size_t sourceLen, std::ostream &);
  std::string Encode(const unsigned char *source, size_t sourceLen);

 private:
  BIO *const m_bio;
};

std::string EncodeToHex(const unsigned char *cource, size_t sourceLen);

boost::array<unsigned char, SHA512_DIGEST_LENGTH> CalcHmacSha512Digest(
    const std::string &source, const std::string &key);
}
}
}