/*******************************************************************************
 *   Created: 2017/10/29 18:36:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Crypto.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::Crypto;

////////////////////////////////////////////////////////////////////////////////

Base64Coder::Base64Coder(bool multiline)
    : m_bio(BIO_push(BIO_new(BIO_f_base64()), BIO_new(BIO_s_mem()))) {
  if (!multiline) {
    BIO_set_flags(m_bio, BIO_FLAGS_BASE64_NO_NL);
  }
}
Base64Coder::~Base64Coder() {
  BIO_set_close(m_bio, BIO_NOCLOSE);
  BIO_free_all(m_bio);
}

void Base64Coder::Encode(const unsigned char *source,
                         size_t sourceLen,
                         std::ostream &stream) {
  BIO_write(m_bio, source, static_cast<int>(sourceLen));
  BIO_flush(m_bio);

  const BUF_MEM *buffer;
  BIO_get_mem_ptr(m_bio, &buffer);
  stream.write(buffer->data, buffer->length);
}

std::string Base64Coder::Encode(const unsigned char *source, size_t sourceLen) {
  BIO_write(m_bio, source, static_cast<int>(sourceLen));
  BIO_flush(m_bio);

  const BUF_MEM *buffer;
  BIO_get_mem_ptr(m_bio, &buffer);
  return {buffer->data, buffer->length};
}

////////////////////////////////////////////////////////////////////////////////

std::string Crypto::EncodeToHex(const unsigned char *source, size_t sourceLen) {
  std::stringstream result;
  result << std::hex;
  for (size_t i = 0; i < sourceLen; ++i) {
    result << std::setw(2) << std::setfill('0')
           << static_cast<int>(*(source + i));
  }
  return result.str();
}

////////////////////////////////////////////////////////////////////////////////

boost::array<unsigned char, SHA512_DIGEST_LENGTH> Crypto::CalcHmacSha512Digest(
    const unsigned char *source, size_t sourceLen, const std::string &key) {
  boost::array<unsigned char, SHA512_DIGEST_LENGTH> result;
  HMAC(EVP_sha512(), key.c_str(), static_cast<int>(key.size()), source,
       sourceLen, &result[0], nullptr);
  return result;
}

boost::array<unsigned char, SHA512_DIGEST_LENGTH> Crypto::CalcHmacSha512Digest(
    const std::string &source, const std::string &key) {
  return CalcHmacSha512Digest(
      reinterpret_cast<const unsigned char *>(source.c_str()), source.size(),
      key);
}

////////////////////////////////////////////////////////////////////////////////
